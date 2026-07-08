#include "Delivery/Traps/DFTrapBase.h"

#include "Character/CharacterCarryComponent.h"
#include "Components/BoxComponent.h"
#include "Components/DFStatusEffectComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/DFTrapDataAsset.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
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

	FGameplayTag ForcedDropEffect()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.Effect.ForcedDrop"), false);
	}

	FGameplayTag PushEffect()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.Effect.Push"), false);
	}

	FGameplayTag ReversePushEffect()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.Effect.ReversePush"), false);
	}

	FGameplayTag InputInvertEffect()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.Effect.InvertInput"), false);
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

	if (!CurrentStateTag.IsValid())
	{
		CurrentStateTag = DFTrapTags::Ready();
	}

	if (TriggerVolume)
	{
		TriggerVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &ADFTrapBase::OnTrapBeginOverlap);
		TriggerVolume->OnComponentEndOverlap.AddUniqueDynamic(this, &ADFTrapBase::OnTrapEndOverlap);
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Trap] BeginPlay: Trap=%s Authority=%d DataAsset=%s Trigger=%s Effect=%s State=%s OverlapEvents=%d Collision=%d PawnResponse=%d"),
		*GetNameSafe(this),
		HasAuthority(),
		*GetNameSafe(TrapDataAsset),
		TrapDataAsset ? *TrapDataAsset->TriggerTypeTag.ToString() : TEXT("None"),
		TrapDataAsset ? *TrapDataAsset->EffectTypeTag.ToString() : TEXT("None"),
		*CurrentStateTag.ToString(),
		TriggerVolume ? TriggerVolume->GetGenerateOverlapEvents() : false,
		TriggerVolume ? static_cast<int32>(TriggerVolume->GetCollisionEnabled()) : -1,
		TriggerVolume ? static_cast<int32>(TriggerVolume->GetCollisionResponseToChannel(ECC_Pawn)) : -1
	);

	if (!TrapDataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] TrapDataAsset is null: %s"), *GetNameSafe(this));
	}
	else
	{
		if (!IsOverlapTrigger())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[Trap] TriggerTypeTag is not Trap.Trigger.Overlap: Trap=%s Trigger=%s"),
				*GetNameSafe(this),
				*TrapDataAsset->TriggerTypeTag.ToString()
			);
		}

		if (!IsKnownEffect())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[Trap] EffectTypeTag is not a known trap effect: Trap=%s Effect=%s"),
				*GetNameSafe(this),
				*TrapDataAsset->EffectTypeTag.ToString()
			);
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
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Trap] BeginOverlap: Trap=%s Other=%s Authority=%d TrapRole=%d OtherRole=%d State=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OtherActor),
		HasAuthority(),
		static_cast<int32>(GetLocalRole()),
		OtherActor ? static_cast<int32>(OtherActor->GetLocalRole()) : -1,
		*CurrentStateTag.ToString()
	);

	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] BeginOverlap ignored on non-authority instance: %s"), *GetNameSafe(this));
		return;
	}

	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	if (!TrapDataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] BeginOverlap ignored: TrapDataAsset is null (%s)"), *GetNameSafe(this));
		return;
	}

	if (!IsOverlapTrigger())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Trap] BeginOverlap ignored: TriggerTypeTag is not Trap.Trigger.Overlap (%s)"),
			*TrapDataAsset->TriggerTypeTag.ToString()
		);
		return;
	}

	if (ShouldRepeatReversePush())
	{
		AddRepeatingReversePushTarget_ServerOnly(OtherActor);

		if (CurrentStateTag.MatchesTagExact(DFTrapTags::Ready()))
		{
			TryActivate(OtherActor);
		}

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

void ADFTrapBase::OnTrapEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex
)
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Trap] EndOverlap: Trap=%s Other=%s Authority=%d"),
		*GetNameSafe(this),
		*GetNameSafe(OtherActor),
		HasAuthority()
	);

	if (!HasAuthority())
	{
		return;
	}

	RemoveRepeatingReversePushTarget_ServerOnly(OtherActor);

	if (ShouldClearInputInvertOnEndOverlap())
	{
		ClearInputInvertEffect_ServerOnly(OtherActor);
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

	if (ShouldRepeatReversePush())
	{
		for (AActor* OverlappingActor : OverlappingActors)
		{
			AddRepeatingReversePushTarget_ServerOnly(OverlappingActor);
		}

		if (PendingActivator && !OverlappingActors.Contains(PendingActivator))
		{
			AddRepeatingReversePushTarget_ServerOnly(PendingActivator);
		}

		return;
	}

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
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] ApplyTrapEffect ignored on non-authority instance: %s"), *GetNameSafe(this));
		return;
	}

	if (!TrapDataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] ApplyTrapEffect failed: TrapDataAsset is null (%s)"), *GetNameSafe(this));
		return;
	}

	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] ApplyTrapEffect failed: TargetActor is null (%s)"), *GetNameSafe(this));
		return;
	}

	if (IsSlowEffect())
	{
		if (!TrapDataAsset->bAffectsPlayer)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Trap] Slow ignored: bAffectsPlayer is false (%s)"), *GetNameSafe(this));
			return;
		}

		if (UDFStatusEffectComponent* StatusEffectComponent =
			TargetActor->FindComponentByClass<UDFStatusEffectComponent>())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[Trap] UDFStatusEffectComponent found: Target=%s Effect=%s Magnitude=%.2f Duration=%.2f"),
				*GetNameSafe(TargetActor),
				*TrapDataAsset->EffectTypeTag.ToString(),
				TrapDataAsset->EffectMagnitude,
				TrapDataAsset->EffectDuration
			);

			StatusEffectComponent->ApplyMoveSpeedModifier(
				TrapDataAsset->EffectTypeTag,
				TrapDataAsset->EffectMagnitude,
				TrapDataAsset->EffectDuration
			);
			return;
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Trap] No UDFStatusEffectComponent on %s"),
			*GetNameSafe(TargetActor)
		);
		return;
	}

	if (IsForcedDropEffect())
	{
		if (!TrapDataAsset->bAffectsPlayer && !TrapDataAsset->bAffectsCarriedBox)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Trap] ForcedDrop ignored: no valid target flags (%s)"), *GetNameSafe(this));
			return;
		}

		ApplyForcedDropEffect_ServerOnly(TargetActor);
		return;
	}

	if (IsPushEffect())
	{
		if (!TrapDataAsset->bAffectsPlayer)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Trap] Push ignored: bAffectsPlayer is false (%s)"), *GetNameSafe(this));
			return;
		}

		ApplyPushEffect_ServerOnly(TargetActor);
		return;
	}

	if (IsReversePushEffect())
	{
		if (!TrapDataAsset->bAffectsPlayer)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Trap] ReversePush ignored: bAffectsPlayer is false (%s)"), *GetNameSafe(this));
			return;
		}

		if (ShouldRepeatReversePush())
		{
			AddRepeatingReversePushTarget_ServerOnly(TargetActor);
			return;
		}

		ApplyReversePushEffect_ServerOnly(TargetActor, false);
		return;
	}

	if (IsInputInvertEffect())
	{
		if (!TrapDataAsset->bAffectsPlayer)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Trap] InvertInput ignored: bAffectsPlayer is false (%s)"), *GetNameSafe(this));
			return;
		}

		ApplyInputInvertEffect_ServerOnly(TargetActor);
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Trap] ApplyTrapEffect ignored: unknown EffectTypeTag (%s)"),
		*TrapDataAsset->EffectTypeTag.ToString()
	);
}

void ADFTrapBase::ApplyForcedDropEffect_ServerOnly(AActor* TargetActor)
{
	if (!HasAuthority() || !TargetActor)
	{
		return;
	}

	UCharacterCarryComponent* CarryComponent = TargetActor->FindComponentByClass<UCharacterCarryComponent>();
	if (!CarryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] No CarryComponent on %s"), *GetNameSafe(TargetActor));
		return;
	}

	if (!CarryComponent->IsCarrying())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] ForcedDrop ignored: target is not carrying (%s)"), *GetNameSafe(TargetActor));
		return;
	}

	const float TrapDamage = TrapDataAsset ? TrapDataAsset->EffectMagnitude : 0.0f;
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Trap] Apply ForcedDrop: Trap=%s Target=%s Damage=%.2f"),
		*GetNameSafe(this),
		*GetNameSafe(TargetActor),
		TrapDamage
	);

	CarryComponent->ForceDropByTrap(TrapDamage);
}

void ADFTrapBase::ApplyPushEffect_ServerOnly(AActor* TargetActor)
{
	if (!HasAuthority() || !TargetActor)
	{
		return;
	}

	ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	if (!TargetCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] Target is not Character: %s"), *GetNameSafe(TargetActor));
		return;
	}

	const FVector Direction = TrapDataAsset && TrapDataAsset->bUseTrapForwardAsPushDirection
		? GetActorForwardVector()
		: TargetCharacter->GetActorForwardVector();

	LaunchCharacterFromTrap_ServerOnly(TargetCharacter, Direction, TEXT("Push"), true, true);
}

void ADFTrapBase::ApplyReversePushEffect_ServerOnly(AActor* TargetActor, bool bRepeated)
{
	if (!HasAuthority() || !TargetActor)
	{
		return;
	}

	ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	if (!TargetCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] Target is not Character: %s"), *GetNameSafe(TargetActor));
		return;
	}

	ApplyReverseGroundPushEffect_ServerOnly(TargetCharacter, bRepeated);
}

void ADFTrapBase::ApplyReverseGroundPushEffect_ServerOnly(ACharacter* TargetCharacter, bool bRepeated)
{
	if (!HasAuthority() || !TargetCharacter)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = TargetCharacter->GetCharacterMovement();
	if (!MovementComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] CharacterMovementComponent is null on %s"), *GetNameSafe(TargetCharacter));
		return;
	}

	const FVector CurrentVelocity = MovementComponent->Velocity;
	const FVector HorizontalVelocity(CurrentVelocity.X, CurrentVelocity.Y, 0.0f);

	FVector Direction = FVector::ZeroVector;
	if (TrapDataAsset && TrapDataAsset->bUseOppositeVelocityForReverse && !HorizontalVelocity.IsNearlyZero(10.0f))
	{
		Direction = -HorizontalVelocity.GetSafeNormal();
	}

	if (Direction.IsNearlyZero())
	{
		Direction = -TargetCharacter->GetActorForwardVector();
		Direction.Z = 0.0f;
		Direction.Normalize();
	}

	if (Direction.IsNearlyZero())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] ReverseGroundPush ignored: direction is zero (%s)"), *GetNameSafe(TargetCharacter));
		return;
	}

	const float PushStrength = TrapDataAsset ? FMath::Max(0.0f, TrapDataAsset->PushStrength) : 0.0f;
	const float MaxPushSpeed = TrapDataAsset ? FMath::Max(0.0f, TrapDataAsset->MaxPushSpeed) : 0.0f;

	FVector NewHorizontalVelocity = HorizontalVelocity + Direction * PushStrength;
	if (MaxPushSpeed > 0.0f)
	{
		NewHorizontalVelocity = NewHorizontalVelocity.GetClampedToMaxSize(MaxPushSpeed);
	}

	MovementComponent->Velocity = FVector(NewHorizontalVelocity.X, NewHorizontalVelocity.Y, CurrentVelocity.Z);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Trap] Apply %s: Trap=%s Target=%s Direction=%s VelocityBefore=%s VelocityAfter=%s MaxPushSpeed=%.2f"),
		bRepeated ? TEXT("ReverseGroundPushRepeat") : TEXT("ReverseGroundPush"),
		*GetNameSafe(this),
		*GetNameSafe(TargetCharacter),
		*Direction.ToString(),
		*CurrentVelocity.ToString(),
		*MovementComponent->Velocity.ToString(),
		MaxPushSpeed
	);

	TargetCharacter->ForceNetUpdate();
}

void ADFTrapBase::ApplyInputInvertEffect_ServerOnly(AActor* TargetActor)
{
	if (!HasAuthority() || !TargetActor)
	{
		return;
	}

	UDFStatusEffectComponent* StatusEffectComponent = TargetActor->FindComponentByClass<UDFStatusEffectComponent>();
	if (!StatusEffectComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] InvertInput failed: target has no UDFStatusEffectComponent (%s)"), *GetNameSafe(TargetActor));
		return;
	}

	const float Duration = TrapDataAsset ? FMath::Max(0.0f, TrapDataAsset->EffectDuration) : 0.0f;
	const FGameplayTag EffectTag = TrapDataAsset ? TrapDataAsset->EffectTypeTag : FGameplayTag();
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Trap] Apply InvertInput: Trap=%s Target=%s Duration=%.2f RemoveOnEndOverlap=%d"),
		*GetNameSafe(this),
		*GetNameSafe(TargetActor),
		Duration,
		TrapDataAsset ? TrapDataAsset->bRemoveEffectOnEndOverlap : false
	);

	StatusEffectComponent->ApplyInputInvert(EffectTag, Duration);
}

void ADFTrapBase::ClearInputInvertEffect_ServerOnly(AActor* TargetActor)
{
	if (!HasAuthority() || !TargetActor)
	{
		return;
	}

	UDFStatusEffectComponent* StatusEffectComponent = TargetActor->FindComponentByClass<UDFStatusEffectComponent>();
	if (!StatusEffectComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] Clear InvertInput failed: target has no UDFStatusEffectComponent (%s)"), *GetNameSafe(TargetActor));
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Trap] Clear InvertInput on EndOverlap: Trap=%s Target=%s"),
		*GetNameSafe(this),
		*GetNameSafe(TargetActor)
	);

	StatusEffectComponent->ClearInputInvert();
}

void ADFTrapBase::LaunchCharacterFromTrap_ServerOnly(
	ACharacter* TargetCharacter,
	const FVector& Direction,
	const TCHAR* EffectName,
	bool bXYOverride,
	bool bZOverride
)
{
	if (!HasAuthority() || !TargetCharacter)
	{
		return;
	}

	if (!TargetCharacter->GetCharacterMovement())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] CharacterMovementComponent is null on %s"), *GetNameSafe(TargetCharacter));
		return;
	}

	const FVector LaunchDirection = Direction.GetSafeNormal();
	if (LaunchDirection.IsNearlyZero())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] %s ignored: launch direction is zero (%s)"), EffectName, *GetNameSafe(TargetCharacter));
		return;
	}

	const float PushStrength = TrapDataAsset ? FMath::Max(0.0f, TrapDataAsset->PushStrength) : 0.0f;
	const float PushUpStrength = TrapDataAsset ? FMath::Max(0.0f, TrapDataAsset->PushUpStrength) : 0.0f;
	const FVector LaunchVelocity = LaunchDirection * PushStrength + FVector::UpVector * PushUpStrength;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Trap] Apply %s: Trap=%s Target=%s Direction=%s LaunchVelocity=%s"),
		EffectName,
		*GetNameSafe(this),
		*GetNameSafe(TargetCharacter),
		*LaunchDirection.ToString(),
		*LaunchVelocity.ToString()
	);

	TargetCharacter->LaunchCharacter(LaunchVelocity, bXYOverride, bZOverride);
	TargetCharacter->ForceNetUpdate();
}

void ADFTrapBase::AddRepeatingReversePushTarget_ServerOnly(AActor* TargetActor)
{
	if (!HasAuthority() || !ShouldRepeatReversePush() || !TargetActor)
	{
		return;
	}

	ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	if (!TargetCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Trap] Target is not Character: %s"), *GetNameSafe(TargetActor));
		return;
	}

	for (const TWeakObjectPtr<ACharacter>& ExistingTarget : RepeatingReversePushTargets)
	{
		if (ExistingTarget.Get() == TargetCharacter)
		{
			StartRepeatEffectTimer_ServerOnly();
			return;
		}
	}

	RepeatingReversePushTargets.Add(TargetCharacter);
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Trap] Add repeating ReversePush target: Trap=%s Target=%s Count=%d"),
		*GetNameSafe(this),
		*GetNameSafe(TargetCharacter),
		RepeatingReversePushTargets.Num()
	);

	StartRepeatEffectTimer_ServerOnly();
}

void ADFTrapBase::RemoveRepeatingReversePushTarget_ServerOnly(AActor* TargetActor)
{
	if (!HasAuthority() || !TargetActor)
	{
		return;
	}

	ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	if (!TargetCharacter)
	{
		return;
	}

	const int32 RemovedCount = RepeatingReversePushTargets.RemoveAll(
		[TargetCharacter](const TWeakObjectPtr<ACharacter>& ExistingTarget)
		{
			return !ExistingTarget.IsValid() || ExistingTarget.Get() == TargetCharacter;
		}
	);

	if (RemovedCount > 0)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Trap] Remove repeating ReversePush target: Trap=%s Target=%s Count=%d"),
			*GetNameSafe(this),
			*GetNameSafe(TargetCharacter),
			RepeatingReversePushTargets.Num()
		);
	}

	if (RepeatingReversePushTargets.Num() == 0)
	{
		StopRepeatEffectTimer_ServerOnly();
	}
}

void ADFTrapBase::StartRepeatEffectTimer_ServerOnly()
{
	if (!HasAuthority() || !ShouldRepeatReversePush() || RepeatingReversePushTargets.Num() == 0)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		if (TimerManager.IsTimerActive(RepeatEffectTimerHandle))
		{
			return;
		}

		const float RepeatInterval = TrapDataAsset ? FMath::Max(0.01f, TrapDataAsset->RepeatInterval) : 0.1f;
		TimerManager.SetTimer(
			RepeatEffectTimerHandle,
			this,
			&ADFTrapBase::ApplyRepeatEffect_ServerOnly,
			RepeatInterval,
			true
		);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Trap] Start repeating ReversePush timer: Trap=%s Interval=%.3f"),
			*GetNameSafe(this),
			RepeatInterval
		);
	}
}

void ADFTrapBase::StopRepeatEffectTimer_ServerOnly()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RepeatEffectTimerHandle);
	}

	UE_LOG(LogTemp, Warning, TEXT("[Trap] Stop repeating ReversePush timer: Trap=%s"), *GetNameSafe(this));
}

void ADFTrapBase::ApplyRepeatEffect_ServerOnly()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!ShouldRepeatReversePush())
	{
		RepeatingReversePushTargets.Empty();
		StopRepeatEffectTimer_ServerOnly();
		return;
	}

	for (int32 TargetIndex = RepeatingReversePushTargets.Num() - 1; TargetIndex >= 0; --TargetIndex)
	{
		ACharacter* TargetCharacter = RepeatingReversePushTargets[TargetIndex].Get();
		if (!IsValid(TargetCharacter) || !TriggerVolume || !TriggerVolume->IsOverlappingActor(TargetCharacter))
		{
			RepeatingReversePushTargets.RemoveAtSwap(TargetIndex);
			continue;
		}

		ApplyReversePushEffect_ServerOnly(TargetCharacter, true);
	}

	if (RepeatingReversePushTargets.Num() == 0)
	{
		StopRepeatEffectTimer_ServerOnly();
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

bool ADFTrapBase::IsForcedDropEffect() const
{
	return TrapDataAsset && TrapDataAsset->EffectTypeTag.MatchesTag(DFTrapTags::ForcedDropEffect());
}

bool ADFTrapBase::IsPushEffect() const
{
	return TrapDataAsset && TrapDataAsset->EffectTypeTag.MatchesTag(DFTrapTags::PushEffect());
}

bool ADFTrapBase::IsReversePushEffect() const
{
	return TrapDataAsset && TrapDataAsset->EffectTypeTag.MatchesTag(DFTrapTags::ReversePushEffect());
}

bool ADFTrapBase::IsInputInvertEffect() const
{
	return TrapDataAsset && TrapDataAsset->EffectTypeTag.MatchesTag(DFTrapTags::InputInvertEffect());
}

bool ADFTrapBase::IsKnownEffect() const
{
	return IsSlowEffect() || IsForcedDropEffect() || IsPushEffect() || IsReversePushEffect() || IsInputInvertEffect();
}

bool ADFTrapBase::ShouldRepeatReversePush() const
{
	return TrapDataAsset
		&& TrapDataAsset->bRepeatWhileOverlapping
		&& TrapDataAsset->bAffectsPlayer
		&& IsOverlapTrigger()
		&& IsReversePushEffect();
}

bool ADFTrapBase::ShouldClearInputInvertOnEndOverlap() const
{
	return TrapDataAsset
		&& IsInputInvertEffect()
		&& TrapDataAsset->bRemoveEffectOnEndOverlap
		&& TrapDataAsset->EffectDuration <= 0.0f;
}
