#include "Character/ParcelInteractionComponent.h"
#include "ParcelLog.h"
#include "Delivery/InteractableInterface.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "CollisionQueryParams.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogParcelInteraction);

UParcelInteractionComponent::UParcelInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    TraceDistance = 300.f;
    CurrentFocusedActor = nullptr;
}

void UParcelInteractionComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // 멀티플레이어 환경에서 빙의(Possession) 타이밍 문제를 방지하기 위해
    // 타이머는 우선 모든 클라이언트에서 돌리되 실제 레이저 연산은 내부에서 로컬 제어권 여부로 필터링
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            TraceTimerHandle, 
            this, 
            &UParcelInteractionComponent::CheckTraceTarget, 
            TraceInterval, 
            true
        );
    }
}

void UParcelInteractionComponent::CheckTraceTarget()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    // 내가 조종하는 로컬 캐릭터 화면이 아니라면 레이저를 쏘지 않고 즉시 리턴
    if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled()) return;

    APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
    if (!PC) return;

    FVector TraceStart;
    FRotator TraceRotation;
    PC->GetPlayerViewPoint(TraceStart, TraceRotation);

    FVector TraceEnd = TraceStart + (TraceRotation.Vector() * TraceDistance);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    
    // 플레이어 자신은 조준에서 제외
    QueryParams.AddIgnoredActor(OwnerCharacter);

    // 이번 프레임에 새로 감지된 액터를 담을 임시 변수
    AActor* NewFocusedActor = nullptr;
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
    {
        AActor* HitActor = HitResult.GetActor();
       
        // 조준된 액터가 인터페이스 규격을 구현했는지 검사
        if (HitActor && HitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
        {
            IInteractableInterface* InteractableTarget = Cast<IInteractableInterface>(HitActor);
            if (InteractableTarget && InteractableTarget->CanInteract(OwnerCharacter))
            {
                // 바로 대입하지 않고 변경점 체크를 위해 임시 변수에 보관
                NewFocusedActor = HitActor;
            }
        }
    }
    
    // 이전 대상과 새로 조준한 대상이 다를 때만(상태 변화 시) 로그 출력 및 상태 업데이트
    if (CurrentFocusedActor != NewFocusedActor)
    {
        if (NewFocusedActor)
        {
            INTERACT_LOG(Log, TEXT("조준 타겟 변경됨. [%s]"), *NewFocusedActor->GetName());
        }
        else if (CurrentFocusedActor)
        {
            INTERACT_LOG(Log, TEXT("조준 타겟 잃음. (이전 대상: %s)"), *CurrentFocusedActor->GetName());
        }
    
        // 새로운 타겟으로 최종 갱신 (아무것도 안 바라면 자연스럽게 nullptr이 들어감)
        CurrentFocusedActor = NewFocusedActor;
    }
}

void UParcelInteractionComponent::PrimaryInteract()
{
    // 로컬 화면상 포커싱된 타겟이 없다면 무시
    if (!CurrentFocusedActor) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    if (CurrentFocusedActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
    {
        IInteractableInterface* InteractableTarget = Cast<IInteractableInterface>(CurrentFocusedActor);
        if (InteractableTarget && InteractableTarget->CanInteract(OwnerCharacter))
        {
            // 호스트 유저 : 즉시 실행
            if (OwnerCharacter->HasAuthority())
            {
                INTERACT_LOG(Log, TEXT("호스트(서버)가 직접 상호작용 실행: [%s]"), *CurrentFocusedActor->GetName());
                InteractableTarget->Interact(OwnerCharacter);
            }
            // 원격 클라이언트 유저 : 서버에 RPC 요청
            else
            {
                INTERACT_LOG(Log, TEXT("클라이언트가 서버에 상호작용 RPC 요청: [%s]"), *CurrentFocusedActor->GetName());
                Server_RequestPrimaryInteract(CurrentFocusedActor);
            }
        }
    }
}
    
// ==========================================================================
// [Server] 멀티플레이 보안용 RPC 레이턴시 보정 거리 검증
// ==========================================================================
    
bool UParcelInteractionComponent::Server_RequestPrimaryInteract_Validate(AActor* TargetActor)
{
    if (!TargetActor) 
    {
        INTERACT_LOG(Warning, TEXT("서버 검증 실패: TargetActor가 nullptr입니다."));
        return false;
    }
    
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) 
    {
        INTERACT_LOG(Warning, TEXT("서버 검증 실패: 소유 캐릭터가 유효하지 않습니다."));
        return false;
    }

    // 패킷 변조 핵 방어: 서버사이드에서 캐릭터와 타겟의 실제 거리를 역계산
    float DistSq = FVector::DistSquared(OwnerCharacter->GetActorLocation(), TargetActor->GetActorLocation());
    float MaxAllowedDistance = TraceDistance + 50.f;
    
    bool bIsValidDistance = DistSq <= FMath::Square(MaxAllowedDistance);

    // 거리 검증 실패 시 불법적인 요청일 수 있으므로 Warning 로그로 기록
    if (!bIsValidDistance)
    {
        INTERACT_LOG(Warning, TEXT("서버 거리 검증 실패! 캐릭터[%s]와 타겟[%s]의 거리가 너무 멉니다. (허용 거리 제곱: %f, 실제 거리 제곱: %f)"), 
            *OwnerCharacter->GetName(), *TargetActor->GetName(), FMath::Square(MaxAllowedDistance), DistSq);
    }

    return bIsValidDistance;
}

void UParcelInteractionComponent::Server_RequestPrimaryInteract_Implementation(AActor* TargetActor)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter || !TargetActor) return;
    
    if (TargetActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
    {
        IInteractableInterface* InteractableTarget = Cast<IInteractableInterface>(TargetActor);
        if (InteractableTarget && InteractableTarget->CanInteract(OwnerCharacter))
        {
            // 서버 월드에서 상자의 상호작용 몸통 로직 실행
            INTERACT_LOG(Log, TEXT("서버에서 캐릭터[%s]의 요청으로 [%s] 상호작용 최종 승인 및 실행"), 
                *OwnerCharacter->GetName(), *TargetActor->GetName());
                
            InteractableTarget->Interact(OwnerCharacter);
        }
    }
}