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
    const FGameplayTag InAirTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.InAir"));

    const bool bIsSprinting = StateComp->HasStateTag(SprintTag);
    const bool bIsExhausted = StateComp->HasStateTag(ExhaustedTag);
    const bool bIsInAir = StateComp->HasStateTag(InAirTag);
    const bool bIsMoving = OwnerChar->GetVelocity().SizeSquared2D() > 10.f;

    bool bIsTimerActive = GetWorld()->GetTimerManager().IsTimerActive(ExhaustionTimerHandle);
    float PreviousStamina = CurrentStamina;

    if (bIsSprinting && bIsMoving && !bIsExhausted)
    {
        // [소모]
        float DrainRate = GetDrainRate();
        CurrentStamina = FMath::Max(0.f, CurrentStamina - (DrainRate * DeltaTime));

        if (CurrentStamina <= 0.f)
        {
            StartExhaustion();
        }
    }
    else if (!bIsTimerActive && !bIsInAir && CurrentStamina < MaxStamina)
    {
        // [회복]
        float RegenRate = GetRegenRate();
        CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + (RegenRate * DeltaTime));
        
        // [탈진 해제]
        if (CurrentStamina >= MaxStamina && bIsExhausted)
        {
            StateComp->RemoveStateTag(ExhaustedTag);
            
            if (UParcelMovementStatComponent* MovementStat = OwnerChar->GetParcelMovementStatComponent())
            {
                MovementStat->RefreshMoveSpeed();
            }
            STAMINA_LOG(All, TEXT("[Server] %s의 스태미나가 100%% 충전되어 탈진 상태가 정상 해제되었습니다."), *OwnerChar->GetName());
        }
    }

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
    STAMINA_LOG(All, TEXT("[Server] 탈진 3초 강제 대기가 완료되었습니다. 스태미나 자연 회복을 개시합니다. (태그는 100%% 충전 시 해제)"));
}

void UParcelStaminaComponent::OnRep_CurrentStamina()
{
    if (OnStaminaChanged.IsBound())
    {
        OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
    }
}