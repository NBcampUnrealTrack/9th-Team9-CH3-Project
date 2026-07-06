#include "Character/ParcelMovementStatComponent.h"
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

    bIsSprinting = false;
    CurrentCarryMultiplier = 1.0f;
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
    ApplyStatsToMovement();
}

void UParcelMovementStatComponent::UpdateDynamicSpeedModifier(bool bInSprinting, float InCarryMultiplier)
{
    bIsSprinting = bInSprinting;
    CurrentCarryMultiplier = InCarryMultiplier;

    // (로컬과 서버 컴포넌트에 주입) 속도 공식 계산식
    float SprintMod = bIsSprinting ? SprintSpeedMultiplier : 1.0f;
    MaxWalkSpeed = BaseMaxWalkSpeed * SprintMod * CurrentCarryMultiplier;
    
    ApplyStatsToMovement();
}

void UParcelMovementStatComponent::SetBaseMaxWalkSpeed(float NewSpeed)
{
    // [Server] 서버 권한이 있을 때만 수정 가능하도록 방어
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;

    BaseMaxWalkSpeed = NewSpeed;
    UpdateDynamicSpeedModifier(bIsSprinting, CurrentCarryMultiplier);
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
