#include "Character/ParcelMovementStatComponent.h"
#include "Character/ParcelCharacter.h"
#include "Character/ParcelPlayerStateComponent.h"
#include "Character/CharacterCarryComponent.h"
#include "Components/DFStatusEffectComponent.h"
#include "ParcelLog.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogParcelMovementStat);

UParcelMovementStatComponent::UParcelMovementStatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    
    BaseMaxWalkSpeed = 450.0f;
    SprintSpeedMultiplier = 2.0f;
    
    MaxWalkSpeed = BaseMaxWalkSpeed;
    JumpZVelocity = 500.f;
    AirControl = 0.35f;
    RotationRate = FRotator(0.f, 540.f, 0.f);
}

void UParcelMovementStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    // 동기화 변수 등록
    DOREPLIFETIME(UParcelMovementStatComponent, MaxWalkSpeed);
    DOREPLIFETIME(UParcelMovementStatComponent, JumpZVelocity);
    DOREPLIFETIME(UParcelMovementStatComponent, AirControl);
    DOREPLIFETIME(UParcelMovementStatComponent, RotationRate);
}

void UParcelMovementStatComponent::BeginPlay()
{
    Super::BeginPlay();
    RefreshMoveSpeed();
}

void UParcelMovementStatComponent::RefreshMoveSpeed()
{
	AParcelCharacter* OwnerCharacter = Cast<AParcelCharacter>(GetOwner());
	if (!OwnerCharacter) return;

	// 최종 속도는 서버가 계산하고 MaxWalkSpeed 복제를 통해 클라이언트에 적용한다.
	if (!OwnerCharacter->HasAuthority())
	{
		ApplyStatsToMovement();
		return;
	}

    // 서버의 현재 Sprint / Carry / Slow 상태를 한 번에 합성한다.
    float SprintMod = 1.0f;
    float CarryMod = 1.0f;
    float StatusEffectMod = 1.0f;

    // PlayerStateComponent에서 달리기 태그 유무 판정
    if (UParcelPlayerStateComponent* StateComp = OwnerCharacter->GetParcelPlayerStateComponent())
    {
        if (StateComp->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Sprinting"))))
        {
            SprintMod = SprintSpeedMultiplier;
        }
    }

    // CharacterCarryComponent에서 상자 감속 배율 획득
    if (UCharacterCarryComponent* CarryComp = OwnerCharacter->GetCharacterCarryComponent())
    {
        CarryMod = CarryComp->GetMoveSpeedMultiplier();
    }

    if (const UDFStatusEffectComponent* StatusEffectComp = OwnerCharacter->FindComponentByClass<UDFStatusEffectComponent>())
    {
        StatusEffectMod = StatusEffectComp->GetMoveSpeedMultiplier();
    }

    MaxWalkSpeed = BaseMaxWalkSpeed * SprintMod * CarryMod * StatusEffectMod;
    
    ApplyStatsToMovement();
}

void UParcelMovementStatComponent::SetBaseMaxWalkSpeed(float NewSpeed)
{
    // [Server] 서버 권한이 있을 때만 수정 가능하도록 방어
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;

    BaseMaxWalkSpeed = NewSpeed;
    RefreshMoveSpeed();
}

// RepNotify 함수
void UParcelMovementStatComponent::OnRep_MaxWalkSpeed() { ApplyStatsToMovement(); }
void UParcelMovementStatComponent::OnRep_JumpZVelocity() { ApplyStatsToMovement(); }
void UParcelMovementStatComponent::OnRep_AirControl() { ApplyStatsToMovement(); }
void UParcelMovementStatComponent::OnRep_RotationRate() { ApplyStatsToMovement(); }

void UParcelMovementStatComponent::ApplyStatsToMovement()
{
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
        {
            Movement->MaxWalkSpeed = MaxWalkSpeed;
            Movement->JumpZVelocity = JumpZVelocity;
            Movement->AirControl = AirControl;
            Movement->RotationRate = RotationRate;
            
            MOVEMENT_LOG(Log, TEXT("[%s] 통합 연산 속도(%f)가 적용되었습니다."), 
                OwnerCharacter->HasAuthority() ? TEXT("Server") : TEXT("Client"), MaxWalkSpeed);
        }
    }
}
