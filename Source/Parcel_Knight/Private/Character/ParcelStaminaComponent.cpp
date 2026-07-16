#include "Character/ParcelStaminaComponent.h"
#include "ParcelLog.h"
#include "Character/ParcelCharacter.h"
#include "Character/ParcelPlayerStateComponent.h"
#include "Character/ParcelMovementStatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogParcelStamina);

UParcelStaminaComponent::UParcelStaminaComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    SetIsReplicatedByDefault(true);
    
    MaxStamina = 100.f;
    CurrentStamina = MaxStamina;
    SprintDrainDuration = 8.0f;
    ExhaustionDuration = 3.0f;
    RegenDuration = 4.0f;
}

void UParcelStaminaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UParcelStaminaComponent, CurrentStamina);
    DOREPLIFETIME(UParcelStaminaComponent, MaxStamina);
}

void UParcelStaminaComponent::BeginPlay()
{
    Super::BeginPlay();
    
    if (GetOwner() && !GetOwner()->HasAuthority())
    {
        PrimaryComponentTick.SetTickFunctionEnable(false);
    }
    else
    {
        CurrentStamina = MaxStamina;
    }
}

void UParcelStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;

    AParcelCharacter* OwnerChar = Cast<AParcelCharacter>(GetOwner());
    if (!OwnerChar) return;

    UParcelPlayerStateComponent* StateComp = OwnerChar->GetParcelPlayerStateComponent();
    if (!StateComp) return;

    const FGameplayTag SprintTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Sprinting"));
    const FGameplayTag ExhaustedTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Exhausted"));

    const bool bIsSprinting = StateComp->HasStateTag(SprintTag);
    const bool bIsExhausted = StateComp->HasStateTag(ExhaustedTag);
    
    // 캐릭터가 전력질주 입력을 켰지만 실제로는 멈춰있을 땐 스태미나를 깎지 않음
    const bool bIsMoving = OwnerChar->GetVelocity().SizeSquared2D() > 10.f;

    float PreviousStamina = CurrentStamina;

    if (bIsSprinting && bIsMoving)
    {
        // 달리는 중 소모
        float DrainRate = GetDrainRate();
        CurrentStamina = FMath::Max(0.f, CurrentStamina - (DrainRate * DeltaTime));

        if (CurrentStamina <= 0.f && !bIsExhausted)
        {
            StartExhaustion();
        }
    }
    else if (!bIsExhausted && CurrentStamina < MaxStamina)
    {
        // 평상시 회복 (탈진 상태가 아니고 전력질주하지 않는 경우)
        float RegenRate = GetRegenRate();
        CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + (RegenRate * DeltaTime));
    }

    // 값에 실질적인 변화가 생겼을 때만 RepNotify 브로드캐스트 (서버 로컬용)
    if (!FMath::IsNearlyEqual(PreviousStamina, CurrentStamina))
    {
        OnRep_CurrentStamina();
    }
}

void UParcelStaminaComponent::StartExhaustion()
{
    AParcelCharacter* OwnerChar = Cast<AParcelCharacter>(GetOwner());
    if (!OwnerChar) return;

    UParcelPlayerStateComponent* StateComp = OwnerChar->GetParcelPlayerStateComponent();
    if (!StateComp) return;
    
    StateComp->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Exhausted")));
    StateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Sprinting")));
    
    if (UParcelMovementStatComponent* MovementStat = OwnerChar->GetParcelMovementStatComponent())
    {
        MovementStat->RefreshMoveSpeed();
    }
    
    GetWorld()->GetTimerManager().SetTimer(
        ExhaustionTimerHandle,
        this,
        &UParcelStaminaComponent::StopExhaustion,
        ExhaustionDuration,
        false
    );

    STAMINA_LOG(All, TEXT("[Server] %s가 스태미나 완전 소진으로 탈진 상태에 진입했습니다."), *OwnerChar->GetName());
}

void UParcelStaminaComponent::StopExhaustion()
{
    AParcelCharacter* OwnerChar = Cast<AParcelCharacter>(GetOwner());
    if (!OwnerChar) return;

    UParcelPlayerStateComponent* StateComp = OwnerChar->GetParcelPlayerStateComponent();
    if (!StateComp) return;
    
    StateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Exhausted")));

    STAMINA_LOG(All, TEXT("[Server] %s의 탈진 상태가 해제되어 스태미나 회복을 개시합니다."), *OwnerChar->GetName());
}

void UParcelStaminaComponent::OnRep_CurrentStamina()
{
    if (OnStaminaChanged.IsBound())
    {
        OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
    }
}