#include "Delivery/DFChickenTrapAI.h"

#include "AIController.h"
#include "Character/CharacterCarryComponent.h"
#include "Character/ParcelCharacter.h"
#include "Animation/AnimInstance.h"
#include "Components/DFKnockbackComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Sound/SoundBase.h"

namespace DFChickenTrapTags
{
	FGameplayTag Ready()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.State.Ready"), false);
	}

	FGameplayTag Disabled()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.State.Disabled"), false);
	}

	FGameplayTag Active()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Trap.State.Active"), false);
	}
}

ADFChickenTrapAI::ADFChickenTrapAI()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	ChickenMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ChickenMesh"));
	RootComponent = ChickenMesh;
	ChickenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChickenMesh->SetGenerateOverlapEvents(false);
	ChickenMesh->SetCanEverAffectNavigation(false);

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->SetupAttachment(ChickenMesh);
	TriggerSphere->InitSphereRadius(120.0f);
	TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerSphere->SetGenerateOverlapEvents(true);
	TriggerSphere->SetCanEverAffectNavigation(false);

	FloatingMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingMovement"));
	FloatingMovement->UpdatedComponent = ChickenMesh;
	FloatingMovement->MaxSpeed = MoveSpeed;
	FloatingMovement->Acceleration = 1800.0f;
	FloatingMovement->Deceleration = 1800.0f;
	FloatingMovement->TurningBoost = 8.0f;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();

	CurrentStateTag = DFChickenTrapTags::Ready();
}

void ADFChickenTrapAI::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!ChickenMesh)
	{
		return;
	}

	if (ChickenSkeletalMesh)
	{
		ChickenMesh->SetSkeletalMesh(ChickenSkeletalMesh);
	}

	if (ChickenAnimClass)
	{
		ChickenMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		ChickenMesh->SetAnimInstanceClass(ChickenAnimClass);
	}
}

void ADFChickenTrapAI::BeginPlay()
{
	Super::BeginPlay();

	if (!CurrentStateTag.IsValid())
	{
		CurrentStateTag = DFChickenTrapTags::Ready();
	}

	if (FloatingMovement)
	{
		FloatingMovement->MaxSpeed = MoveSpeed;
	}

	PatrolOriginLocation = GetActorLocation();

	if (TriggerSphere)
	{
		TriggerSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ADFChickenTrapAI::OnTriggerBeginOverlap);
	}

	if (HasAuthority())
	{
		StartAIMovement_ServerOnly();
	}
}

void ADFChickenTrapAI::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADFChickenTrapAI, CurrentStateTag);
	DOREPLIFETIME(ADFChickenTrapAI, bHasTriggeredOnce);
}

FGameplayTag ADFChickenTrapAI::GetTrapStateTag_Implementation() const
{
	return CurrentStateTag;
}

bool ADFChickenTrapAI::IsTrapReady_Implementation() const
{
	return CurrentStateTag.MatchesTagExact(DFChickenTrapTags::Ready());
}

bool ADFChickenTrapAI::RequestActivate_Implementation(AActor* Activator)
{
	return ActivateTrap(Activator);
}

bool ADFChickenTrapAI::CanActivate_Implementation(AActor* Activator) const
{
	return HasAuthority() && CanActivate_ServerOnly(Activator);
}

void ADFChickenTrapAI::Server_RequestActivate_Implementation(AActor* Activator)
{
	TryActivate(Activator);
}

bool ADFChickenTrapAI::TryActivate(AActor* Activator)
{
	return ActivateTrap(Activator);
}

bool ADFChickenTrapAI::ActivateTrap(AActor* InstigatorActor)
{
	if (!HasAuthority())
	{
		Server_RequestActivate(InstigatorActor);
		return false;
	}

	if (!CanActivate_ServerOnly(InstigatorActor))
	{
		return false;
	}

	ActivateTrap_ServerOnly(InstigatorActor);
	return true;
}

void ADFChickenTrapAI::ActivateTrap_ServerOnly(AActor* Activator)
{
	if (!HasAuthority() || !CanActivate_ServerOnly(Activator))
	{
		return;
	}

	AParcelCharacter* HitCharacter = Cast<AParcelCharacter>(Activator);
	if (!HitCharacter)
	{
		return;
	}

	bHasTriggeredOnce = true;
	SetTrapState_ServerOnly(DFChickenTrapTags::Active());
	StopAIMovement_ServerOnly();

	if (TriggerSphere)
	{
		TriggerSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TriggerSphere->SetGenerateOverlapEvents(false);
	}

	const FVector ExplosionLocation = GetActorLocation();
	Multicast_PlayExplosionFX(ExplosionLocation);

	ApplyKnockback_ServerOnly(HitCharacter);
	DropCarriedBox_ServerOnly(HitCharacter);
	SetTrapState_ServerOnly(DFChickenTrapTags::Disabled());

	if (DestroyDelay <= 0.0f)
	{
		Destroy();
	}
	else
	{
		SetLifeSpan(DestroyDelay);
	}

	ForceNetUpdate();
}

void ADFChickenTrapAI::Multicast_PlayExplosionFX_Implementation(FVector_NetQuantize ExplosionLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, ExplosionEffect, ExplosionLocation);
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ExplosionLocation);
	}
}

void ADFChickenTrapAI::OnTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (!HasAuthority())
	{
		return;
	}

	ActivateTrap(OtherActor);
}

void ADFChickenTrapAI::StartAIMovement_ServerOnly()
{
	if (!HasAuthority() || !bEnableServerRoaming || bHasTriggeredOnce || MoveSpeed <= 0.0f)
	{
		return;
	}

	UpdateAIMovement_ServerOnly();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			AIMoveTimerHandle,
			this,
			&ADFChickenTrapAI::UpdateAIMovement_ServerOnly,
			FMath::Max(0.1f, MoveUpdateInterval),
			true
		);
	}
}

void ADFChickenTrapAI::UpdateAIMovement_ServerOnly()
{
	if (!HasAuthority() || bHasTriggeredOnce)
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		return;
	}

	if (MoveToClosestPlayer_ServerOnly())
	{
		return;
	}

	if (AIController->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		MoveToRandomReachablePoint_ServerOnly();
	}
}

bool ADFChickenTrapAI::MoveToClosestPlayer_ServerOnly()
{
	if (!HasAuthority() || bHasTriggeredOnce)
	{
		return false;
	}

	AParcelCharacter* ClosestPlayer = FindClosestPlayerInDetectionRadius_ServerOnly();
	if (!ClosestPlayer)
	{
		return false;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		return false;
	}

	AIController->MoveToActor(
		ClosestPlayer,
		MoveAcceptanceRadius,
		true,
		true,
		true,
		nullptr,
		true
	);
	return true;
}

void ADFChickenTrapAI::MoveToRandomReachablePoint_ServerOnly()
{
	if (!HasAuthority() || bHasTriggeredOnce || PatrolRadius <= 0.0f)
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSystem)
	{
		return;
	}

	FNavLocation RoamLocation;
	if (NavSystem->GetRandomReachablePointInRadius(PatrolOriginLocation, PatrolRadius, RoamLocation))
	{
		AIController->MoveToLocation(
			RoamLocation.Location,
			MoveAcceptanceRadius,
			true,
			true,
			true,
			true,
			nullptr,
			true
		);
	}
}

AParcelCharacter* ADFChickenTrapAI::FindClosestPlayerInDetectionRadius_ServerOnly() const
{
	if (!HasAuthority() || DetectionRadius <= 0.0f)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> PlayerActors;
	UGameplayStatics::GetAllActorsOfClass(World, AParcelCharacter::StaticClass(), PlayerActors);

	AParcelCharacter* ClosestPlayer = nullptr;
	float ClosestDistanceSquared = FMath::Square(DetectionRadius);
	const FVector CurrentLocation = GetActorLocation();

	for (AActor* PlayerActor : PlayerActors)
	{
		AParcelCharacter* PlayerCharacter = Cast<AParcelCharacter>(PlayerActor);
		if (!PlayerCharacter)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(CurrentLocation, PlayerCharacter->GetActorLocation());
		if (DistanceSquared <= ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestPlayer = PlayerCharacter;
		}
	}

	return ClosestPlayer;
}

bool ADFChickenTrapAI::CanActivate_ServerOnly(AActor* Activator) const
{
	return HasAuthority()
		&& CurrentStateTag.MatchesTagExact(DFChickenTrapTags::Ready())
		&& !bHasTriggeredOnce
		&& Cast<AParcelCharacter>(Activator) != nullptr;
}

void ADFChickenTrapAI::SetTrapState_ServerOnly(FGameplayTag NewStateTag)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentStateTag = NewStateTag;
	OnRep_CurrentState();
	ForceNetUpdate();
}

void ADFChickenTrapAI::OnRep_CurrentState()
{
	if (CurrentStateTag.MatchesTagExact(DFChickenTrapTags::Disabled()) && TriggerSphere)
	{
		TriggerSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TriggerSphere->SetGenerateOverlapEvents(false);
	}
}

void ADFChickenTrapAI::ApplyKnockback_ServerOnly(AParcelCharacter* HitCharacter) const
{
	if (!HasAuthority() || !HitCharacter)
	{
		return;
	}

	if (UDFKnockbackComponent* KnockbackComponent = HitCharacter->FindComponentByClass<UDFKnockbackComponent>())
	{
		KnockbackComponent->ApplyKnockbackFromLocation(GetActorLocation(), KnockbackStrength, UpwardStrength);
		return;
	}

	ApplyLaunchCharacterFallback_ServerOnly(HitCharacter);
}

void ADFChickenTrapAI::ApplyLaunchCharacterFallback_ServerOnly(AParcelCharacter* HitCharacter) const
{
	if (!HasAuthority() || !HitCharacter)
	{
		return;
	}

	FVector Direction = HitCharacter->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.0f;

	if (!Direction.Normalize())
	{
		Direction = GetActorForwardVector();
		Direction.Z = 0.0f;
		Direction.Normalize();
	}

	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FVector LaunchVelocity = Direction * KnockbackStrength + FVector(0.0f, 0.0f, UpwardStrength);
	HitCharacter->LaunchCharacter(LaunchVelocity, true, true);
	HitCharacter->ForceNetUpdate();
}

void ADFChickenTrapAI::DropCarriedBox_ServerOnly(AParcelCharacter* HitCharacter) const
{
	if (!HasAuthority() || !bDropCarriedBoxOnHit || !HitCharacter)
	{
		return;
	}

	UCharacterCarryComponent* CarryComponent = HitCharacter->GetCharacterCarryComponent();
	if (CarryComponent && CarryComponent->IsCarrying())
	{
		CarryComponent->Drop();
	}
}

void ADFChickenTrapAI::StopAIMovement_ServerOnly()
{
	if (!HasAuthority())
	{
		return;
	}

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AIMoveTimerHandle);
	}
}
