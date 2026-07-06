#include "Character/CharacterCarryComponent.h"
#include "Character/ParcelMovementStatComponent.h"
#include "Character/ParcelCharacter.h"
#include "Character/ParcelPlayerStateComponent.h"
#include "Components/ActorComponent.h" 
#include "GameFramework/Character.h"
#include "Delivery/DeliveryBox.h"
#include "Delivery/CarryableInterface.h"
#include "Delivery/PhysicsJudgeManager.h"
#include "Net/UnrealNetwork.h"

UCharacterCarryComponent::UCharacterCarryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

    CarriedBox = nullptr;
    bIsCarrying = false;
    MoveSpeedMultiplier = 1.0f;
    HandSocketName = TEXT("HandSocket");
}

void UCharacterCarryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UCharacterCarryComponent, CarriedBox);
    DOREPLIFETIME(UCharacterCarryComponent, bIsCarrying);
}

// BeginPlay에 있던 '속도'관련 함수를 MovementStatComponent에서 통합 관리하도록 이전시켰습니다

void UCharacterCarryComponent::Pickup(ADeliveryBox* InBox)
{
    if (!GetOwner()->HasAuthority() || !InBox) return;

    CarriedBox = InBox;
    bIsCarrying = true;
    MoveSpeedMultiplier = InBox->GetBoxData().MoveSpeedMultiplier;

    // 상자를 잡는 순간, 캐릭터 상태 컴포넌트에 Carrying 태그 추가
    if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(GetOwner()))
    {
        if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
        {
            StateComp->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Carrying")));
        }
    }

    // 서버 사이드 물리적 처리 및 상태 전파용 강제 트리거 호출
    OnRep_CarriedBox();
    SyncWeightToMovement();
}

void UCharacterCarryComponent::Drop()
{
    if (!GetOwner()->HasAuthority())
    {
       Server_Drop();
       return;
    }

    if (!CarriedBox) return;

    if (ICarryableInterface* Carryable = Cast<ICarryableInterface>(CarriedBox))
    {
       Carryable->OnDropped();
    }

    CarriedBox = nullptr;
    bIsCarrying = false;
    MoveSpeedMultiplier = 1.0f;

    // 상자를 바닥에 내려놓는 순간 Carrying 태그 해제 및 제거
    if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(GetOwner()))
    {
        if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
        {
            StateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Carrying")));
        }
    }

    OnRep_CarriedBox();
    SyncWeightToMovement();
}

void UCharacterCarryComponent::Throw(FVector Force)
{
    if (!GetOwner()->HasAuthority())
    {
       Server_Throw(Force);
       return;
    }

    if (!CarriedBox) return;

    ADeliveryBox* BoxToThrow = CarriedBox;

    if (ICarryableInterface* Carryable = Cast<ICarryableInterface>(BoxToThrow))
    {
       Carryable->OnDropped();
    }

    CarriedBox = nullptr;
    bIsCarrying = false;
    MoveSpeedMultiplier = 1.0f;

    // 상자를 멀리 던지는 순간에도 Carrying 태그 회수
    if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(GetOwner()))
    {
        if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
        {
            StateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Carrying")));
        }
    }

    OnRep_CarriedBox();
    SyncWeightToMovement();

    // 지연 분리 시점에 월드 임펄스 물리 적용
    if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(BoxToThrow->GetRootComponent()))
    {
       RootPrim->AddImpulse(Force, NAME_None, true);
    }
}

void UCharacterCarryComponent::ForceDropByTrap(float TrapDamage)
{
    if (!GetOwner()->HasAuthority() || !CarriedBox) return;

    ADeliveryBox* BoxActor = CarriedBox;
    Drop();

    if (UWorld* World = GetWorld())
    {
       if (UPhysicsJudgeManager* JudgeManager = World->GetSubsystem<UPhysicsJudgeManager>())
       {
          JudgeManager->EvaluateTrapImpact(BoxActor, TrapDamage);
       }
    }
}

void UCharacterCarryComponent::OnRep_CarriedBox()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    if (CarriedBox)
    {
    	// 상자 자체의 물리/충돌 컴포넌트가 복제되기 전에 클라이언트가 통제(레이턴시 해결)
    	if (UPrimitiveComponent* BoxRootPrim = Cast<UPrimitiveComponent>(CarriedBox->GetRootComponent()))
    	{
    		BoxRootPrim->SetSimulatePhysics(false);
    		BoxRootPrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    	}
    	// [상자를 잡았을 때] 시각적 부착 처리
       FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
       CarriedBox->AttachToComponent(OwnerCharacter->GetMesh(), AttachmentRules, HandSocketName);

       // [Add] : 현재 잡은 상자를 기록해 둬야, 나중에 손에서 뗄 때 서버와의 nullptr 괴리를 방지할 수 있음
       PreviousCarriedBox = CarriedBox;
    }
    else
    {
       if (PreviousCarriedBox && PreviousCarriedBox->IsValidLowLevel())
       {
          // Transform 보존하고 바인딩을 해제함
          FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, true);
          PreviousCarriedBox->DetachFromActor(DetachRules);

          // 물리 및 콜리전 복구
          if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(PreviousCarriedBox->GetRootComponent()))
          {
             RootPrim->SetSimulatePhysics(true);
             RootPrim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
          }

          // 캐시 초기화
          PreviousCarriedBox = nullptr;
       }
    }
}

void UCharacterCarryComponent::SyncWeightToMovement()
{
    if (UParcelMovementStatComponent* StatComp = GetOwner()->FindComponentByClass<UParcelMovementStatComponent>())
    {
       // 무브먼트 컴포넌트에 무게 정보를 갱신
       StatComp->RefreshMoveSpeed();
    }
}


// [Server]
bool UCharacterCarryComponent::Server_Drop_Validate() { return true; }
void UCharacterCarryComponent::Server_Drop_Implementation() { Drop(); }
bool UCharacterCarryComponent::Server_Throw_Validate(FVector Force) { return true; }
void UCharacterCarryComponent::Server_Throw_Implementation(FVector Force) { Throw(Force); }