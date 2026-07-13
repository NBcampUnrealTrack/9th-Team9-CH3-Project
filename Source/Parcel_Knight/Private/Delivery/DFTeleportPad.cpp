#include "Delivery/DFTeleportPad.h"

#include "Character/CharacterCarryComponent.h"
#include "Character/ParcelCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Delivery/DeliveryBox.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDFTeleportPad, Log, All);

namespace DFTeleportPadTags
{
	FGameplayTag Ready()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.State.Ready"), false);
	}

	FGameplayTag Disabled()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.State.Disabled"), false);
	}
}

ADFTeleportPad::ADFTeleportPad()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadMesh"));
	PadMesh->SetupAttachment(SceneRoot);
	PadMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PadMesh->SetCollisionObjectType(ECC_WorldStatic);
	PadMesh->SetCollisionResponseToAllChannels(ECR_Block);
	PadMesh->SetGenerateOverlapEvents(false);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(SceneRoot);
	TriggerBox->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	TriggerBox->SetBoxExtent(FVector(100.0f, 100.0f, 50.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
}

void ADFTeleportPad::BeginPlay()
{
	Super::BeginPlay();

	if (!TriggerBox)
	{
		UE_LOG(LogDFTeleportPad, Error, TEXT("TeleportPad: TriggerBox is null. Pad=%s"), *GetNameSafe(this));
		return;
	}

	// Blueprint instances can retain older component defaults, so enforce the required trigger settings at runtime.
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ADFTeleportPad::OnTriggerBeginOverlap);

	UE_LOG(
		LogDFTeleportPad,
		Warning,
		TEXT("TeleportPad: BeginPlay Pad=%s Authority=%d TriggerBound=%d Collision=%d PawnResponse=%d GenerateOverlap=%d TriggerLocation=%s TriggerExtent=%s Destination=%s"),
		*GetNameSafe(this),
		HasAuthority(),
		TriggerBox->OnComponentBeginOverlap.IsBound(),
		static_cast<int32>(TriggerBox->GetCollisionEnabled()),
		static_cast<int32>(TriggerBox->GetCollisionResponseToChannel(ECC_Pawn)),
		TriggerBox->GetGenerateOverlapEvents(),
		*TriggerBox->GetComponentLocation().ToString(),
		*TriggerBox->GetScaledBoxExtent().ToString(),
		*GetNameSafe(DestinationActor)
	);
}

FGameplayTag ADFTeleportPad::GetTrapStateTag_Implementation() const
{
	return IsValid(DestinationActor) ? DFTeleportPadTags::Ready() : DFTeleportPadTags::Disabled();
}

bool ADFTeleportPad::IsTrapReady_Implementation() const
{
	return IsValid(DestinationActor);
}

bool ADFTeleportPad::RequestActivate_Implementation(AActor* Activator)
{
	return ActivateTeleportPad(Activator);
}

bool ADFTeleportPad::CanActivate_Implementation(AActor* Activator) const
{
	return CanActivate_ServerOnly(Activator);
}

void ADFTeleportPad::OnTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	UE_LOG(
		LogDFTeleportPad,
		Warning,
		TEXT("TeleportPad: Overlap detected with %s. Pad=%s OtherComp=%s Authority=%d"),
		*GetNameSafe(OtherActor),
		*GetNameSafe(this),
		*GetNameSafe(OtherComp),
		HasAuthority()
	);

	TryTeleportActor(OtherActor);
}

bool ADFTeleportPad::ActivateTeleportPad(AActor* Activator)
{
	return TryTeleportActor(Activator);
}

bool ADFTeleportPad::TryTeleportActor(AActor* ActorToTeleport)
{
	UE_LOG(
		LogDFTeleportPad,
		Warning,
		TEXT("TeleportPad: TryTeleportActor Target=%s Authority=%d UseServerAuthority=%d"),
		*GetNameSafe(ActorToTeleport),
		HasAuthority(),
		bUseServerAuthority
	);

	AParcelCharacter* PlayerCharacter = Cast<AParcelCharacter>(ActorToTeleport);
	if (!IsValid(PlayerCharacter))
	{
		UE_LOG(
			LogDFTeleportPad,
			Warning,
			TEXT("TeleportPad: Character cast failed for %s"),
			*GetNameSafe(ActorToTeleport)
		);
		return false;
	}

	const UCapsuleComponent* CapsuleComponent = PlayerCharacter->GetCapsuleComponent();
	UE_LOG(
		LogDFTeleportPad,
		Warning,
		TEXT("TeleportPad: Character cast succeeded. Player=%s Capsule=%s CapsuleGenerateOverlap=%d CapsuleResponseToTrigger=%d"),
		*GetNameSafe(PlayerCharacter),
		*GetNameSafe(CapsuleComponent),
		CapsuleComponent ? CapsuleComponent->GetGenerateOverlapEvents() : false,
		CapsuleComponent && TriggerBox
			? static_cast<int32>(CapsuleComponent->GetCollisionResponseToChannel(TriggerBox->GetCollisionObjectType()))
			: -1
	);

	if (!HasAuthority())
	{
		if (bUseServerAuthority)
		{
			if (HasLocalNetOwner())
			{
				UE_LOG(LogDFTeleportPad, Warning, TEXT("TeleportPad: Sending Server_RequestTeleport for %s"), *GetNameSafe(PlayerCharacter));
				Server_RequestTeleport(PlayerCharacter);
			}
			else
			{
				UE_LOG(
					LogDFTeleportPad,
					Warning,
					TEXT("TeleportPad: Client overlap cannot call Server RPC because this placed pad has no local net owner; waiting for authoritative server overlap. Player=%s"),
					*GetNameSafe(PlayerCharacter)
				);
			}
		}

		return false;
	}

	if (!IsValid(DestinationActor))
	{
		UE_LOG(
			LogDFTeleportPad,
			Warning,
			TEXT("TeleportPad: DestinationActor is null or invalid. Pad=%s Player=%s"),
			*GetNameSafe(this),
			*GetNameSafe(PlayerCharacter)
		);
		return false;
	}

	UE_LOG(
		LogDFTeleportPad,
		Warning,
		TEXT("TeleportPad: DestinationActor is valid. Destination=%s"),
		*GetNameSafe(DestinationActor)
	);

	if (!CanActivate_ServerOnly(PlayerCharacter))
	{
		UE_LOG(
			LogDFTeleportPad,
			Warning,
			TEXT("TeleportPad: Ignored because player is on cooldown. Player=%s"),
			*GetNameSafe(PlayerCharacter)
		);
		return false;
	}

	return TeleportPlayer_ServerOnly(PlayerCharacter);
}

void ADFTeleportPad::Server_RequestTeleport_Implementation(AActor* ActorToTeleport)
{
	UE_LOG(
		LogDFTeleportPad,
		Warning,
		TEXT("TeleportPad: Server_RequestTeleport received. Target=%s Authority=%d"),
		*GetNameSafe(ActorToTeleport),
		HasAuthority()
	);

	if (!TriggerBox || !TriggerBox->IsOverlappingActor(ActorToTeleport))
	{
		UE_LOG(
			LogDFTeleportPad,
			Warning,
			TEXT("TeleportPad: Server request rejected because target is not overlapping TriggerBox. Target=%s"),
			*GetNameSafe(ActorToTeleport)
		);
		return;
	}

	TryTeleportActor(ActorToTeleport);
}

bool ADFTeleportPad::CanActivate_ServerOnly(AActor* Activator) const
{
	if (!HasAuthority() || !IsValid(DestinationActor) || !Cast<AParcelCharacter>(Activator))
	{
		return false;
	}

	return !RecentlyTeleportedActors.Contains(TWeakObjectPtr<AActor>(Activator));
}

bool ADFTeleportPad::TeleportPlayer_ServerOnly(AParcelCharacter* PlayerCharacter)
{
	if (!HasAuthority() || !IsValid(PlayerCharacter) || !IsValid(DestinationActor))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogDFTeleportPad, Warning, TEXT("Teleport failed: World is null. Pad=%s"), *GetNameSafe(this));
		return false;
	}

	const FVector SourceLocation = PlayerCharacter->GetActorLocation();
	const FVector TargetLocation = DestinationActor->GetActorLocation();
	const FRotator TargetRotation = DestinationActor->GetActorRotation();

	UCharacterCarryComponent* CarryComponent = PlayerCharacter->GetCharacterCarryComponent();
	ADeliveryBox* CarriedBox = CarryComponent && CarryComponent->IsCarrying()
		? CarryComponent->GetCarriedBox()
		: nullptr;
	const FTransform CarriedBoxSourceTransform = IsValid(CarriedBox)
		? CarriedBox->GetActorTransform()
		: FTransform::Identity;

	if (bResetVelocityOnTeleport)
	{
		if (UCharacterMovementComponent* MovementComponent = PlayerCharacter->GetCharacterMovement())
		{
			MovementComponent->Velocity = FVector::ZeroVector;
			MovementComponent->StopMovementImmediately();
		}
	}

	const bool bTeleported = PlayerCharacter->TeleportTo(TargetLocation, TargetRotation, false, false);
	UE_LOG(
		LogDFTeleportPad,
		Warning,
		TEXT("TeleportPad: Teleport result = %s. Player=%s TargetLocation=%s TargetRotation=%s"),
		bTeleported ? TEXT("Success") : TEXT("Failed"),
		*GetNameSafe(PlayerCharacter),
		*TargetLocation.ToString(),
		*TargetRotation.ToString()
	);

	if (!bTeleported)
	{
		UE_LOG(
			LogDFTeleportPad,
			Warning,
			TEXT("Teleport failed: Pad=%s Player=%s Destination=%s Location=%s Rotation=%s"),
			*GetNameSafe(this),
			*GetNameSafe(PlayerCharacter),
			*GetNameSafe(DestinationActor),
			*TargetLocation.ToString(),
			*TargetRotation.ToString()
		);
		return false;
	}

	if (!bTeleportCarriedBox && IsValid(CarriedBox) && CarryComponent)
	{
		CarryComponent->Drop();

		const bool bRestoredBox = CarriedBox->SetActorLocationAndRotation(
			CarriedBoxSourceTransform.GetLocation(),
			CarriedBoxSourceTransform.Rotator(),
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);

		if (!bRestoredBox)
		{
			UE_LOG(
				LogDFTeleportPad,
				Warning,
				TEXT("Carried box could not be restored to the source location. Pad=%s Box=%s"),
				*GetNameSafe(this),
				*GetNameSafe(CarriedBox)
			);
		}
	}

	const float CooldownDuration = FMath::Max(0.0f, TeleportCooldown);
	if (CooldownDuration > 0.0f)
	{
		RecentlyTeleportedActors.Add(PlayerCharacter);

		const TWeakObjectPtr<AActor> WeakPlayer(PlayerCharacter);
		FTimerDelegate CooldownDelegate;
		CooldownDelegate.BindWeakLambda(this, [this, WeakPlayer]()
		{
			ClearTeleportCooldown(WeakPlayer.Get());
		});

		FTimerHandle CooldownTimerHandle;
		World->GetTimerManager().SetTimer(CooldownTimerHandle, CooldownDelegate, CooldownDuration, false);
	}

	PlayerCharacter->ForceNetUpdate();
	Multicast_PlayTeleportFX(SourceLocation, PlayerCharacter->GetActorLocation());
	return true;
}

void ADFTeleportPad::ClearTeleportCooldown(AActor* ActorToClear)
{
	if (!HasAuthority())
	{
		return;
	}

	if (ActorToClear)
	{
		RecentlyTeleportedActors.Remove(TWeakObjectPtr<AActor>(ActorToClear));
		UE_LOG(
			LogDFTeleportPad,
			Warning,
			TEXT("TeleportPad: Cooldown cleared. Player=%s"),
			*GetNameSafe(ActorToClear)
		);
	}

	for (auto CooldownIt = RecentlyTeleportedActors.CreateIterator(); CooldownIt; ++CooldownIt)
	{
		if (!CooldownIt->IsValid())
		{
			CooldownIt.RemoveCurrent();
		}
	}
}

void ADFTeleportPad::Multicast_PlayTeleportFX_Implementation(
	FVector StartLocation,
	FVector EndLocation
)
{
	if (TeleportEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, TeleportEffect, StartLocation);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, TeleportEffect, EndLocation);
	}

	if (TeleportSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, TeleportSound, StartLocation);
		UGameplayStatics::PlaySoundAtLocation(this, TeleportSound, EndLocation);
	}
}
