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
    
    // 캐릭터 속도 관련 스탯 기본값
    MaxWalkSpeed = 450.0f;
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

    // 서버와 클라이언트 모두 각자의 로컬 캐릭터 무브먼트에 기본값 주입
    ApplyStatsToMovement();
}

void UParcelMovementStatComponent::SetMaxWalkSpeed(float NewSpeed)
{
    // [Server] 서버 권한이 있을 때만 수정 가능하도록 방어
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;

    MaxWalkSpeed = NewSpeed;
    
    // [Server] RepNotify가 로컬에서 자동으로 호출되지 않으므로 수동으로 적용 함수 실행
    ApplyStatsToMovement();
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
            
            MOVEMENT_LOG(Log, TEXT("[%s] 이동 스탯이 캐릭터에게 적용되었습니다."), 
                OwnerCharacter->HasAuthority() ? TEXT("Server") : TEXT("Client"));
        }
    }
}
