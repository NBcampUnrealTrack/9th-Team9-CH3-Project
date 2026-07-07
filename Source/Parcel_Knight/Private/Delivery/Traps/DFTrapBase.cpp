#include "Delivery/Traps/DFTrapBase.h"

#include "Components/BoxComponent.h"
#include "Components/DFStatusEffectComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/DFTrapDataAsset.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

namespace DFTrapTags
{
	FGameplayTag Ready()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.State.Ready"), false);
	}

	FGameplayTag Warning()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.State.Warning"), false);
	}

	FGameplayTag Active()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.State.Active"), false);
	}

	FGameplayTag Cooldown()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.State.Cooldown"), false);
	}

	FGameplayTag Disabled()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.State.Disabled"), false);
	}

	FGameplayTag OverlapTrigger()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.Trigger.Overlap"), false);
	}

	FGameplayTag SlowEffect()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.Effect.Slow"), false);
	}
}

ADFTrapBase::ADFTrapBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(SceneRoot);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(SceneRoot);
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);

	CurrentStateTag = DFTrapTags::Ready();
}

void ADFTrapBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (!CurrentStateTag.IsValid())
		{
			CurrentStateTag = DFTrapTags::Ready();
		}

		if (TriggerVolume)
		{
			TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ADFTrapBase::OnTrapBeginOverlap);
		}
	}
}

void ADFTrapBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADFTrapBase, CurrentStateTag);
	DOREPLIFETIME(ADFTrapBase, bHasTriggeredOnce);
}

FGameplayTag ADFTrapBase::GetTrapStateTag_Implementation() const
{
	return CurrentStateTag;
}

bool ADFTrapBase::IsTrapReady_Implementation() const
{
	return CurrentStateTag.MatchesTagExact(DFTrapTags::Ready());
}

bool ADFTrapBase::RequestActivate_Implementation(AActor* Activator)
{
	return TryActivate(Activator);
}

bool ADFTrapBase::CanActivate_Implementation(AActor* Activator) const
{
	return HasAuthority() && CanActivate_ServerOnly(Activator);
}

void ADFTrapBase::Server_RequestActivate_Implementation(AActor* Activator)
{
	TryActivate(Activator);
}

bool ADFTrapBase::TryActivate(AActor* Activator)
{
	if (!HasAuthority())
	{
		Server_RequestActivate(Activator);
		return false;
	}

	if (!CanActivate_ServerOnly(Activator))
	{
		return false;
	}

	ActivateTrap_ServerOnly(Activator);
	return true;
}

void ADFTrapBase::ActivateTrap_ServerOnly(AActor* Activator)
{
	if (!HasAuthority() || !CanActivate_ServerOnly(Activator))
	{
		return;
	}

	PendingActivator = Activator;
	bHasTriggeredOnce = true;

	const float WarningTime = TrapDataAsset ? FMath::Max(0.0f, TrapDataAsset->WarningTime) : 0.0f;
	if (WarningTime > 0.0f)
	{
		SetTrapState_ServerOnly(DFTrapTags::Warning());

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				WarningTimerHandle,
				this,
				&ADFTrapBase::EnterActiveState_ServerOnly,
				WarningTime,
				false
			);
		}
	}
	else
	{
		EnterActiveState_ServerOnly();
	}
}

void ADFTrapBase::ResetTrap_ServerOnly()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WarningTimerHandle);
		World->GetTimerManager().ClearTimer(ActiveTimerHandle);
		World->GetTimerManager().ClearTimer(CooldownTimerHandle);
	}

	PendingActivator = nullptr;
	SetTrapState_ServerOnly(DFTrapTags::Ready());
	Multicast_PlayResetFX();
}

void ADFTrapBase::Multicast_PlayActivateFX_Implementation()
{
	if (TrapDataAsset)
	{
		if (TrapDataAsset->ActivateVFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TrapDataAsset->ActivateVFX, GetActorTransform());
		}

		if (TrapDataAsset->ActivateSFX)
		{
			UGameplayStatics::PlaySoundAtLocation(this, TrapDataAsset->ActivateSFX, GetActorLocation());
		}
	}

	OnTrapActivatedVisual();
}

void ADFTrapBase::Multicast_PlayResetFX_Implementation()
{
	if (TrapDataAsset)
	{
		if (TrapDataAsset->ResetVFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TrapDataAsset->ResetVFX, GetActorTransform());
		}

		if (TrapDataAsset->ResetSFX)
		{
			UGameplayStatics::PlaySoundAtLocation(this, TrapDataAsset->ResetSFX, GetActorLocation());
		}
	}

	OnTrapResetVisual();
}

void ADFTrapBase::OnTrapBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (!HasAuthority() || !OtherActor || OtherActor == this || !IsOverlapTrigger())
	{
		return;
	}

	if (CurrentStateTag.MatchesTagExact(DFTrapTags::Ready()))
	{
		TryActivate(OtherActor);
		return;
	}

	if (CurrentStateTag.MatchesTagExact(DFTrapTags::Active()))
	{
		ApplyTrapEffect_ServerOnly(OtherActor);
	}
}

void ADFTrapBase::OnRep_CurrentState()
{
}

bool ADFTrapBase::CanActivate_ServerOnly(AActor* Activator) const
{
	if (!HasAuthority() || !TrapDataAsset || !Activator)
	{
		return false;
	}

	if (!CurrentStateTag.MatchesTagExact(DFTrapTags::Ready()))
	{
		return false;
	}

	if (TrapDataAsset->bTriggerOnce && bHasTriggeredOnce)
	{
		return false;
	}

	return true;
}

void ADFTrapBase::SetTrapState_ServerOnly(FGameplayTag NewStateTag)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentStateTag = NewStateTag;
	ForceNetUpdate();
}

void ADFTrapBase::EnterActiveState_ServerOnly()
{
	if (!HasAuthority())
	{
		return;
	}

	SetTrapState_ServerOnly(DFTrapTags::Active());
	Multicast_PlayActivateFX();
	ApplyTrapEffectToOverlappingActors_ServerOnly();

	const float ActiveDuration = TrapDataAsset ? FMath::Max(0.0f, TrapDataAsset->ActiveDuration) : 0.0f;
	if (ActiveDuration > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				ActiveTimerHandle,
				this,
				&ADFTrapBase::EnterCooldownState_ServerOnly,
				ActiveDuration,
				false
			);
		}
	}
	else
	{
		EnterCooldownState_ServerOnly();
	}
}

void ADFTrapBase::EnterCooldownState_ServerOnly()
{
	if (!HasAuthority())
	{
		return;
	}

	PendingActivator = nullptr;

	if (TrapDataAsset && TrapDataAsset->bTriggerOnce)
	{
		SetTrapState_ServerOnly(DFTrapTags::Disabled());
		return;
	}

	SetTrapState_ServerOnly(DFTrapTags::Cooldown());

	const float Cooldown = TrapDataAsset ? FMath::Max(0.0f, TrapDataAsset->Cooldown) : 0.0f;
	if (Cooldown > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				CooldownTimerHandle,
				this,
				&ADFTrapBase::ResetTrap_ServerOnly,
				Cooldown,
				false
			);
		}
	}
	else
	{
		ResetTrap_ServerOnly();
	}
}

void ADFTrapBase::ApplyTrapEffectToOverlappingActors_ServerOnly()
{
	if (!HasAuthority() || !TriggerVolume)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	TriggerVolume->GetOverlappingActors(OverlappingActors);

	for (AActor* OverlappingActor : OverlappingActors)
	{
		ApplyTrapEffect_ServerOnly(OverlappingActor);
	}

	if (PendingActivator && !OverlappingActors.Contains(PendingActivator))
	{
		ApplyTrapEffect_ServerOnly(PendingActivator);
	}
}

void ADFTrapBase::ApplyTrapEffect_ServerOnly(AActor* TargetActor)
{
	if (!HasAuthority() || !TrapDataAsset || !TargetActor)
	{
		return;
	}

	if (TrapDataAsset->bAffectsPlayer)
	{
		if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
		{
			if (IsSlowEffect())
			{
				if (UDFStatusEffectComponent* StatusEffectComponent =
					TargetCharacter->FindComponentByClass<UDFStatusEffectComponent>())
				{
					StatusEffectComponent->ApplyMoveSpeedModifier(
						TrapDataAsset->EffectTypeTag,
						TrapDataAsset->EffectMagnitude,
						TrapDataAsset->EffectDuration
					);
				}
			}
		}
	}
}

bool ADFTrapBase::IsOverlapTrigger() const
{
	return TrapDataAsset && TrapDataAsset->TriggerTypeTag.MatchesTagExact(DFTrapTags::OverlapTrigger());
}

bool ADFTrapBase::IsSlowEffect() const
{
	return TrapDataAsset && TrapDataAsset->EffectTypeTag.MatchesTag(DFTrapTags::SlowEffect());
}
