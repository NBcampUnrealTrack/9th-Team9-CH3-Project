#include "Character/ParcelCharacter.h"
#include "ParcelLog.h"
#include "Character/RagdollComponent.h"
#include "Character/ParcelHeroComponent.h"
#include "Character/ParcelInteractionComponent.h"
#include "Character/ParcelMovementStatComponent.h"
#include "Character/CharacterCarryComponent.h"
#include "Character/ParcelPlayerStateComponent.h"
#include "Character/ParcelStaminaComponent.h"
#include "Components/DFKnockbackComponent.h"
#include "Components/DFStatusEffectComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Components/WidgetComponent.h"
#include "Core/HealthComponent.h"
#include "Core/ParcelPlayerState.h"
#include "UI/ParcelNameplateWidget.h"

AParcelCharacter::AParcelCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicateMovement(true);
    bReplicates = true;

    SetNetUpdateFrequency(33.f);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->AirControl = 0.35f;

    PlayerStateComp = CreateDefaultSubobject<UParcelPlayerStateComponent>(TEXT("PlayerStateComp"));
    RagdollComp = CreateDefaultSubobject<URagdollComponent>(TEXT("RagdollComp"));
    StatusEffectComponent = CreateDefaultSubobject<UDFStatusEffectComponent>(TEXT("StatusEffectComponent"));
    KnockbackComponent = CreateDefaultSubobject<UDFKnockbackComponent>(TEXT("KnockbackComponent"));
    HeroComp = CreateDefaultSubobject<UParcelHeroComponent>(TEXT("HeroComp"));
    InteractionComp = CreateDefaultSubobject<UParcelInteractionComponent>(TEXT("InteractionComp"));
    MovementStatComp = CreateDefaultSubobject<UParcelMovementStatComponent>(TEXT("MovementStatComp"));
    CarryComp = CreateDefaultSubobject<UCharacterCarryComponent>(TEXT("CarryComp"));
    StaminaComp = CreateDefaultSubobject<UParcelStaminaComponent>(TEXT("StaminaComp"));
    
    NameplateWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameplateWidgetComp"));
    if (NameplateWidgetComp)
    {
       NameplateWidgetComp->SetupAttachment(GetMesh());
       NameplateWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
       NameplateWidgetComp->SetDrawSize(FVector2D(250.f, 80.f));
       NameplateWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 210.f));
    }
    if (PlayerStateComp)
    {
       PlayerStateComp->OnCharacterStateTagsChanged.AddUniqueDynamic(this, &AParcelCharacter::OnCharacterStateTagsChanged);
    }

    bIsRagdoll = false;
    bIsGettingUp = false;
}

void AParcelCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    if (PlayerStateComp)
    {
       PlayerStateComp->OnCharacterStateTagsChanged.AddUniqueDynamic(this, &AParcelCharacter::OnCharacterStateTagsChanged);
    }
    
    if (UHealthComponent* HealthComp = FindComponentByClass<UHealthComponent>())
    {
       HealthComp->OnDeathDelegate.RemoveDynamic(this, &AParcelCharacter::HandleCharacterDeath);
       HealthComp->OnDeathDelegate.AddUniqueDynamic(this, &AParcelCharacter::HandleCharacterDeath);
    }
    
    if (GetWorld())
    {
       FTimerHandle StandaloneNameplateTimer;
       GetWorldTimerManager().SetTimer(StandaloneNameplateTimer, this, &AParcelCharacter::UpdateOverheadNameplate, 0.2f, false);
    }
}

void AParcelCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
    Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

    if (HasAuthority() && PlayerStateComp)
    {
       FGameplayTag InAirTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.InAir"));
       if (GetCharacterMovement()->MovementMode == MOVE_Falling)
       {
          PlayerStateComp->AddStateTag(InAirTag);
       }
    }
}

void AParcelCharacter::OnJumped_Implementation()
{
    Super::OnJumped_Implementation();
    if (HasAuthority() && PlayerStateComp)
    {
       PlayerStateComp->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.InAir")));
    }
}

void AParcelCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    if (HasAuthority() && PlayerStateComp)
    {
       FGameplayTag InAirTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.InAir"));
       if (PlayerStateComp->HasStateTag(InAirTag)) PlayerStateComp->RemoveStateTag(InAirTag);
       PlayerStateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.Action.Jump")));
    }
    if (MovementStatComp) MovementStatComp->RefreshMoveSpeed();
}

void AParcelCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (HeroComp) HeroComp->InitializePlayerInput(PlayerInputComponent);
}

void AParcelCharacter::SetRagdollState(bool bNewIsRagdoll, bool bNewIsGettingUp)
{
    bIsRagdoll = bNewIsRagdoll;
    bIsGettingUp = bNewIsGettingUp;
    
    if (HasAuthority() && PlayerStateComp)
    {
       FGameplayTag RagdollTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Ragdoll"));
       FGameplayTag GettingUpTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.GettingUp"));

       if (bIsRagdoll) PlayerStateComp->AddStateTag(RagdollTag);
       else            PlayerStateComp->RemoveStateTag(RagdollTag);

       if (bIsGettingUp) PlayerStateComp->AddStateTag(GettingUpTag);
       else              PlayerStateComp->RemoveStateTag(GettingUpTag);
    }

    if (bIsGettingUp)
    {
       GetWorldTimerManager().ClearTimer(GetUpTimerHandle);
       GetWorldTimerManager().SetTimer(GetUpTimerHandle, this, &AParcelCharacter::FinishGetUp, 1.3f, false);
    }
    else
    {
       GetWorldTimerManager().ClearTimer(GetUpTimerHandle);
    }
}

void AParcelCharacter::FinishGetUp()
{
    bIsGettingUp = false;
    if (HasAuthority() && PlayerStateComp)
    {
       PlayerStateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.GettingUp")));
    }
}

void AParcelCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (HeroComp) HeroComp->AddInputMappingContext();
    UpdateOverheadNameplate();
}

void AParcelCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    UpdateOverheadNameplate();
}

void AParcelCharacter::UpdateOverheadNameplate()
{
    APlayerState* PS = GetPlayerState();
    if (!PS)
    {
       if (!NameplateRetryTimerHandle.IsValid() && GetWorld())
       {
          GetWorldTimerManager().SetTimer(NameplateRetryTimerHandle, this, &AParcelCharacter::UpdateOverheadNameplate, 0.1f, false);
       }
       return;
    }

    if (NameplateWidgetComp)
    {
       if (UParcelNameplateWidget* NameWidget = Cast<UParcelNameplateWidget>(NameplateWidgetComp->GetUserWidgetObject()))
       {
          FString Nickname = PS->GetPlayerName();
          NameWidget->SetPlayerName(Nickname);
          if (PlayerStateComp) NameWidget->UpdateStatusEffects(PlayerStateComp->GetCharacterStateTags());
          GetWorldTimerManager().ClearTimer(NameplateRetryTimerHandle);
       }
       else
       {
          if (!NameplateRetryTimerHandle.IsValid() && GetWorld())
          {
             GetWorldTimerManager().SetTimer(NameplateRetryTimerHandle, this, &AParcelCharacter::UpdateOverheadNameplate, 0.1f, false);
          }
       }
    }
}

void AParcelCharacter::OnCharacterStateTagsChanged(const FGameplayTagContainer& ActiveTags)
{
    if (NameplateWidgetComp)
    {
       if (UParcelNameplateWidget* NameWidget = Cast<UParcelNameplateWidget>(NameplateWidgetComp->GetUserWidgetObject()))
       {
          NameWidget->UpdateStatusEffects(ActiveTags);
       }
    }
}

void AParcelCharacter::OnRep_Controller()
{
    Super::OnRep_Controller();
    if (HeroComp) HeroComp->AddInputMappingContext();
}

void AParcelCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AParcelCharacter::HandleCharacterDeath()
{
    if (RagdollComp) RagdollComp->StartRagdoll();
    if (HasAuthority() && PlayerStateComp)
    {
       PlayerStateComp->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Dead")));
    }
    if (HasAuthority())
    {
       if (AParcelPlayerState* PS = GetPlayerState<AParcelPlayerState>()) PS->HandleDeath();
    }
}