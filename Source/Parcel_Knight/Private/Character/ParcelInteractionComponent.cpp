#include "Character/ParcelInteractionComponent.h"
#include "ParcelLog.h"
#include "Delivery/InteractableInterface.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "CollisionQueryParams.h"
#include "TimerManager.h"
#include "Character/ParcelCharacter.h"
#include "Character/ParcelPlayerStateComponent.h"
#include "Core/HealthComponent.h"
#include "GameplayTagContainer.h"

DEFINE_LOG_CATEGORY(LogParcelInteraction);

UParcelInteractionComponent::UParcelInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    TraceDistance = 800.f;
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
    
    if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(OwnerCharacter))
    {
        if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
        {
            if (StateComp->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Ragdoll"))))
            {
                if (CurrentFocusedActor != nullptr)
                {
                    INTERACT_LOG(Log, TEXT("래그돌 상태가 감지되어 조준 중이던 타겟을 강제 해제합니다."));
                    CurrentFocusedActor = nullptr;
                }
                return;
            }
        }
    }
    
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
    
    FCollisionShape SweepSphere = FCollisionShape::MakeSphere(15.f); // 15cm SweepSingleByChannel로 변경

    if (GetWorld()->SweepSingleByChannel(HitResult, TraceStart, TraceEnd, FQuat::Identity, ECC_Visibility, SweepSphere, QueryParams))
    {
        AActor* HitActor = HitResult.GetActor();
   
        if (HitActor && HitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
        {
            if (IInteractableInterface::Execute_CanInteract(HitActor, OwnerCharacter))
            {
                NewFocusedActor = HitActor;
            }
            else
            {
                // [디버그] 만약 1, 2단계는 통과했는데 여기서 막힌다면 상자의 태그(Spawned) 상태가 클라에 복제가 안 된 것임
                INTERACT_LOG(Warning, TEXT("[Client Trace] %s 의 CanInteract 조건문 검사에서 탈락함(태그 공백 의심)"), *HitActor->GetName());
            }
        }
    }
    
    // 이전 대상과 새로 조준한 대상이 다를 때만(상태 변화 시) 상태 업데이트
    if (CurrentFocusedActor != NewFocusedActor)
    {
        CurrentFocusedActor = NewFocusedActor;
        OnFocusChanged.Broadcast(CurrentFocusedActor);
    }
}

void UParcelInteractionComponent::PrimaryInteract()
{
    // 로컬 화면상 포커싱된 타겟이 없다면 무시
    if (!CurrentFocusedActor) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(OwnerCharacter))
    {
        if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
        {
            if (StateComp->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Ragdoll"))))
            {
                INTERACT_LOG(Warning, TEXT("래그돌 상태에서는 상호작용 입력을 처리할 수 없습니다."));
                return;
            }
        }
    }
    
    if (CurrentFocusedActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
    {
        if (IInteractableInterface::Execute_CanInteract(CurrentFocusedActor, OwnerCharacter))
        {
            // 호스트 유저 : 즉시 실행
            if (OwnerCharacter->HasAuthority())
            {
                INTERACT_LOG(Log, TEXT("호스트(서버)가 직접 상호작용 실행: [%s]"), *CurrentFocusedActor->GetName());
                IInteractableInterface::Execute_Interact(CurrentFocusedActor, OwnerCharacter);
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
	// 일반적인 잘못된 요청은 연결 종료가 아니라 Implementation에서 거절한다.
	return true;
}

void UParcelInteractionComponent::Server_RequestPrimaryInteract_Implementation(AActor* TargetActor)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !IsValidServerInteractionRequest(TargetActor)) return;
    
    if (TargetActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
    {
        if (IInteractableInterface::Execute_CanInteract(TargetActor, OwnerCharacter))
        {
            // 서버 월드에서 상자의 상호작용 몸통 로직 실행
            INTERACT_LOG(Log, TEXT("서버에서 캐릭터[%s]의 요청으로 [%s] 상호작용 최종 승인 및 실행"), 
                *OwnerCharacter->GetName(), *TargetActor->GetName());
                
            IInteractableInterface::Execute_Interact(TargetActor, OwnerCharacter);
        }
    }
}

bool UParcelInteractionComponent::IsValidServerInteractionRequest(AActor* TargetActor) const
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!IsValid(TargetActor) || !OwnerCharacter || TargetActor == OwnerCharacter || !GetWorld())
	{
		return false;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		return false;
	}

	if (const UHealthComponent* HealthComponent = OwnerCharacter->FindComponentByClass<UHealthComponent>())
	{
		if (HealthComponent->IsDead())
		{
			return false;
		}
	}

	if (const AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(OwnerCharacter))
	{
		if (const UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
		{
			if (StateComp->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Ragdoll"))))
			{
				return false;
			}
		}
	}

	if (!FMath::IsFinite(TraceDistance) || TraceDistance <= 0.0f)
	{
		return false;
	}

	const float MaxAllowedDistance = TraceDistance + 50.0f;
	const float DistanceSquared = FVector::DistSquared(OwnerCharacter->GetActorLocation(), TargetActor->GetActorLocation());
	if (!FMath::IsFinite(DistanceSquared) || DistanceSquared > FMath::Square(MaxAllowedDistance))
	{
		return false;
	}

	APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PlayerController)
	{
		return false;
	}

	const FVector TraceStart = OwnerCharacter->GetPawnViewLocation();
	const FRotator TraceRotation = PlayerController->GetControlRotation();
	const FVector TraceDirection = TraceRotation.Vector();
	if (TraceStart.ContainsNaN() || TraceDirection.ContainsNaN())
	{
		return false;
	}

	const FVector TraceEnd = TraceStart + TraceDirection * MaxAllowedDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	FHitResult HitResult;
	const FCollisionShape SweepSphere = FCollisionShape::MakeSphere(15.0f);
	if (!GetWorld()->SweepSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_Visibility,
		SweepSphere,
		QueryParams))
	{
		return false;
	}

	return HitResult.GetActor() == TargetActor;
}
