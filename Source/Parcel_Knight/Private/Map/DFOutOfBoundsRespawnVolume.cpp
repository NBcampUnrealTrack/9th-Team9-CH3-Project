#include "Map/DFOutOfBoundsRespawnVolume.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogDFOutOfBoundsRespawn);

ADFOutOfBoundsRespawnVolume::ADFOutOfBoundsRespawnVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetActorHiddenInGame(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	RespawnTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("RespawnTrigger"));
	RespawnTrigger->SetupAttachment(SceneRoot);
	RespawnTrigger->SetBoxExtent(FVector(1000.0f, 1000.0f, 100.0f));
	RespawnTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RespawnTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	RespawnTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	RespawnTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RespawnTrigger->SetGenerateOverlapEvents(true);
}

void ADFOutOfBoundsRespawnVolume::BeginPlay()
{
	Super::BeginPlay();

	if (!RespawnTrigger)
	{
		UE_LOG(
			LogDFOutOfBoundsRespawn,
			Error,
			TEXT("OutOfBounds: RespawnTrigger is null. Volume=%s"),
			*GetNameSafe(this)
		);
		return;
	}

	if (!HasAuthority())
	{
		RespawnTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UE_LOG(
			LogDFOutOfBoundsRespawn,
			Verbose,
			TEXT("OutOfBounds: Client trigger disabled. Volume=%s Authority=0"),
			*GetNameSafe(this)
		);
		return;
	}

	// Blueprint instances can retain older component defaults, so enforce the server trigger settings at runtime.
	RespawnTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RespawnTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	RespawnTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	RespawnTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RespawnTrigger->SetGenerateOverlapEvents(true);
	RespawnTrigger->OnComponentBeginOverlap.AddUniqueDynamic(
		this,
		&ADFOutOfBoundsRespawnVolume::OnTriggerBeginOverlap
	);

	UE_LOG(
		LogDFOutOfBoundsRespawn,
		Log,
		TEXT("OutOfBounds: Ready. Volume=%s IsLobby=%d Authority=%d Target=%s Overview=%s"),
		*GetNameSafe(this),
		bIsLobby,
		HasAuthority(),
		*GetNameSafe(RespawnTargetActor),
		*GetNameSafe(OverviewCameraActor)
	);
}

void ADFOutOfBoundsRespawnVolume::OnTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	UE_LOG(
		LogDFOutOfBoundsRespawn,
		Warning,
		TEXT("OutOfBounds: Overlap detected Player=%s IsLobby=%d Authority=%d"),
		*GetNameSafe(OtherActor),
		bIsLobby,
		HasAuthority()
	);

	if (!HasAuthority())
	{
		UE_LOG(LogDFOutOfBoundsRespawn, Warning, TEXT("OutOfBounds: Ignored non-authority overlap"));
		return;
	}

	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!IsValid(Character))
	{
		UE_LOG(
			LogDFOutOfBoundsRespawn,
			Warning,
			TEXT("OutOfBounds: Ignored non-character actor=%s"),
			*GetNameSafe(OtherActor)
		);
		return;
	}

	if (!Character->IsPlayerControlled())
	{
		UE_LOG(
			LogDFOutOfBoundsRespawn,
			Warning,
			TEXT("OutOfBounds: Ignored non-player character=%s"),
			*GetNameSafe(Character)
		);
		return;
	}

	const TWeakObjectPtr<AActor> CharacterKey(Character);
	if (PendingRespawnActors.Contains(CharacterKey))
	{
		UE_LOG(
			LogDFOutOfBoundsRespawn,
			Warning,
			TEXT("OutOfBounds: Duplicate overlap ignored. Player=%s"),
			*GetNameSafe(Character)
		);
		return;
	}

	HandleOutOfBounds(Character);
}

void ADFOutOfBoundsRespawnVolume::HandleOutOfBounds(ACharacter* Character)
{
	if (!HasAuthority() || !IsValid(Character))
	{
		return;
	}

	UE_LOG(
		LogDFOutOfBoundsRespawn,
		Warning,
		TEXT("OutOfBounds: Handle player=%s IsLobby=%d TargetValid=%d OverviewValid=%d"),
		*GetNameSafe(Character),
		bIsLobby,
		IsValid(RespawnTargetActor),
		IsValid(OverviewCameraActor)
	);

	if (bIsLobby)
	{
		PendingRespawnActors.Add(Character);
		RespawnImmediately(Character);
		return;
	}

	BeginDelayedRespawn(Character);
}

void ADFOutOfBoundsRespawnVolume::RespawnImmediately(ACharacter* Character)
{
	if (!HasAuthority() || !IsValid(Character))
	{
		ClearPendingRespawn(Character);
		return;
	}

	UE_LOG(
		LogDFOutOfBoundsRespawn,
		Warning,
		TEXT("OutOfBounds: Immediate respawn. Player=%s"),
		*GetNameSafe(Character)
	);

	FTransform RespawnTransform;
	if (GetRespawnTransform(Character->GetController(), RespawnTransform))
	{
		TeleportCharacter(Character, RespawnTransform);
	}

	ClearPendingRespawn(Character);
}

void ADFOutOfBoundsRespawnVolume::BeginDelayedRespawn(ACharacter* Character)
{
	if (!HasAuthority() || !IsValid(Character))
	{
		return;
	}

	PendingRespawnActors.Add(Character);
	AController* Controller = Character->GetController();
	APlayerController* PlayerController = Cast<APlayerController>(Controller);

	UE_LOG(
		LogDFOutOfBoundsRespawn,
		Warning,
		TEXT("OutOfBounds: Begin delayed respawn. Player=%s Delay=%.2f"),
		*GetNameSafe(Character),
		FMath::Max(0.0f, StageRespawnDelay)
	);

	ApplyOverviewCamera(PlayerController);
	SetCharacterWaitingState(Character, true);

	const TWeakObjectPtr<ACharacter> CharacterPtr(Character);
	const TWeakObjectPtr<AController> ControllerPtr(Controller);
	const float Delay = FMath::Max(0.0f, StageRespawnDelay);
	if (Delay <= 0.0f)
	{
		FinishDelayedRespawn(CharacterPtr, ControllerPtr);
		return;
	}

	FTimerDelegate RespawnDelegate;
	RespawnDelegate.BindUObject(
		this,
		&ADFOutOfBoundsRespawnVolume::FinishDelayedRespawn,
		CharacterPtr,
		ControllerPtr
	);

	FTimerHandle RespawnTimerHandle;
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, RespawnDelegate, Delay, false);
}

void ADFOutOfBoundsRespawnVolume::FinishDelayedRespawn(
	TWeakObjectPtr<ACharacter> CharacterPtr,
	TWeakObjectPtr<AController> ControllerPtr
)
{
	if (!HasAuthority())
	{
		return;
	}

	ACharacter* Character = CharacterPtr.Get();
	AController* Controller = ControllerPtr.Get();
	if (!IsValid(Character))
	{
		UE_LOG(LogDFOutOfBoundsRespawn, Warning, TEXT("OutOfBounds: Delayed respawn target is no longer valid"));
		ClearPendingRespawn(nullptr);
		return;
	}
	if (!IsValid(Controller))
	{
		Controller = Character->GetController();
	}

	FTransform RespawnTransform;
	if (GetRespawnTransform(Controller, RespawnTransform))
	{
		TeleportCharacter(Character, RespawnTransform);
	}

	SetCharacterWaitingState(Character, false);
	RestorePlayerCamera(Cast<APlayerController>(Controller), Character);
	ClearPendingRespawn(Character);

	UE_LOG(
		LogDFOutOfBoundsRespawn,
		Warning,
		TEXT("OutOfBounds: Delayed respawn finished. Player=%s"),
		*GetNameSafe(Character)
	);
}

bool ADFOutOfBoundsRespawnVolume::GetRespawnTransform(
	AController* Controller,
	FTransform& OutTransform
) const
{
	if (IsValid(RespawnTargetActor))
	{
		OutTransform = RespawnTargetActor->GetActorTransform();
		OutTransform.AddToTranslation(FVector(0.0f, 0.0f, FMath::Max(0.0f, RespawnZOffset)));
		UE_LOG(
			LogDFOutOfBoundsRespawn,
			Log,
			TEXT("OutOfBounds: Using RespawnTargetActor=%s"),
			*GetNameSafe(RespawnTargetActor)
		);
		return true;
	}

	AActor* PlayerStart = nullptr;
	if (IsValid(Controller))
	{
		if (AGameModeBase* GameMode = UGameplayStatics::GetGameMode(this))
		{
			PlayerStart = GameMode->FindPlayerStart(Controller);
		}
	}

	if (!IsValid(PlayerStart))
	{
		PlayerStart = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass());
	}
	if (IsValid(PlayerStart))
	{
		OutTransform = PlayerStart->GetActorTransform();
		OutTransform.AddToTranslation(FVector(0.0f, 0.0f, FMath::Max(0.0f, RespawnZOffset)));
		UE_LOG(
			LogDFOutOfBoundsRespawn,
			Warning,
			TEXT("OutOfBounds: RespawnTargetActor is null. Using PlayerStart=%s"),
			*GetNameSafe(PlayerStart)
		);
		return true;
	}

	UE_LOG(
		LogDFOutOfBoundsRespawn,
		Error,
		TEXT("OutOfBounds: RespawnTargetActor and PlayerStart are invalid. Keeping current location. Volume=%s"),
		*GetNameSafe(this)
	);
	return false;
}

void ADFOutOfBoundsRespawnVolume::ApplyOverviewCamera(APlayerController* PlayerController)
{
	if (!HasAuthority() || !IsValid(PlayerController))
	{
		UE_LOG(
			LogDFOutOfBoundsRespawn,
			Warning,
			TEXT("OutOfBounds: Overview camera skipped. PlayerController=%s"),
			*GetNameSafe(PlayerController)
		);
		return;
	}

	if (!IsValid(OverviewCameraActor))
	{
		UE_LOG(LogDFOutOfBoundsRespawn, Warning, TEXT("OutOfBounds: OverviewCameraActor is null; delay continues without camera switch"));
		return;
	}

	FViewTargetTransitionParams TransitionParams;
	TransitionParams.BlendTime = FMath::Max(0.0f, CameraBlendTime);
	PlayerController->ClientSetViewTarget(OverviewCameraActor, TransitionParams);

	UE_LOG(
		LogDFOutOfBoundsRespawn,
		Warning,
		TEXT("OutOfBounds: Overview camera applied. Controller=%s Camera=%s Blend=%.2f"),
		*GetNameSafe(PlayerController),
		*GetNameSafe(OverviewCameraActor),
		TransitionParams.BlendTime
	);
}

void ADFOutOfBoundsRespawnVolume::RestorePlayerCamera(
	APlayerController* PlayerController,
	ACharacter* Character
)
{
	if (!HasAuthority() || !IsValid(PlayerController) || !IsValid(Character))
	{
		UE_LOG(
			LogDFOutOfBoundsRespawn,
			Warning,
			TEXT("OutOfBounds: Player camera restore skipped. Controller=%s Character=%s"),
			*GetNameSafe(PlayerController),
			*GetNameSafe(Character)
		);
		return;
	}

	FViewTargetTransitionParams TransitionParams;
	TransitionParams.BlendTime = FMath::Max(0.0f, CameraBlendTime);
	PlayerController->ClientSetViewTarget(Character, TransitionParams);

	UE_LOG(
		LogDFOutOfBoundsRespawn,
		Warning,
		TEXT("OutOfBounds: Player camera restored. Controller=%s Character=%s Blend=%.2f"),
		*GetNameSafe(PlayerController),
		*GetNameSafe(Character),
		TransitionParams.BlendTime
	);
}

void ADFOutOfBoundsRespawnVolume::SetCharacterWaitingState(ACharacter* Character, bool bWaiting)
{
	if (!HasAuthority() || !IsValid(Character))
	{
		return;
	}

	const TWeakObjectPtr<AActor> CharacterKey(Character);
	if (bWaiting)
	{
		PreviousHiddenStates.FindOrAdd(CharacterKey) = Character->IsHidden();

		if (bDisableMovementWhileWaiting)
		{
			if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
			{
				MovementComponent->StopMovementImmediately();
				MovementComponent->DisableMovement();
			}
		}

		if (bHidePlayerWhileWaiting)
		{
			Character->SetActorHiddenInGame(true);
		}

		return;
	}

	if (const bool* bWasHidden = PreviousHiddenStates.Find(CharacterKey))
	{
		Character->SetActorHiddenInGame(*bWasHidden);
	}

	if (bDisableMovementWhileWaiting)
	{
		if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
		{
			MovementComponent->SetMovementMode(MOVE_Walking);
		}
	}
}

void ADFOutOfBoundsRespawnVolume::ClearPendingRespawn(AActor* Actor)
{
	if (Actor)
	{
		const TWeakObjectPtr<AActor> ActorKey(Actor);
		PendingRespawnActors.Remove(ActorKey);
		PreviousHiddenStates.Remove(ActorKey);
	}

	for (auto PendingIt = PendingRespawnActors.CreateIterator(); PendingIt; ++PendingIt)
	{
		if (!PendingIt->IsValid())
		{
			PendingIt.RemoveCurrent();
		}
	}

	for (auto HiddenIt = PreviousHiddenStates.CreateIterator(); HiddenIt; ++HiddenIt)
	{
		if (!HiddenIt.Key().IsValid())
		{
			HiddenIt.RemoveCurrent();
		}
	}
}

bool ADFOutOfBoundsRespawnVolume::TeleportCharacter(
	ACharacter* Character,
	const FTransform& RespawnTransform
)
{
	if (!HasAuthority() || !IsValid(Character))
	{
		return false;
	}

	if (bResetVelocityOnRespawn)
	{
		if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
		{
			MovementComponent->Velocity = FVector::ZeroVector;
			MovementComponent->StopMovementImmediately();
		}
	}

	const FVector TargetLocation = RespawnTransform.GetLocation();
	const FRotator TargetRotation = RespawnTransform.Rotator();
	const bool bTeleported = Character->TeleportTo(TargetLocation, TargetRotation, false, false);
	Character->ForceNetUpdate();

	UE_LOG(
		LogDFOutOfBoundsRespawn,
		Warning,
		TEXT("OutOfBounds: Respawn teleport result=%s Player=%s TargetLocation=%s TargetRotation=%s"),
		bTeleported ? TEXT("Success") : TEXT("Failed"),
		*GetNameSafe(Character),
		*TargetLocation.ToString(),
		*TargetRotation.ToString()
	);

	if (!bTeleported)
	{
		UE_LOG(
			LogDFOutOfBoundsRespawn,
			Error,
			TEXT("OutOfBounds: TeleportTo failed. Player=%s Volume=%s"),
			*GetNameSafe(Character),
			*GetNameSafe(this)
		);
	}

	return bTeleported;
}
