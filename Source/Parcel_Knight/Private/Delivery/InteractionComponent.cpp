#include "Delivery/InteractionComponent.h"
#include "Delivery/Carryable.h"
#include "Delivery/CarryComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "CollisionQueryParams.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	TraceDistance = 300.f;
	CurrentFocusedActor = nullptr;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	CheckTraceTarget();
}

void UInteractionComponent::CheckTraceTarget()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC) return;

	FVector TraceStart;
	FRotator TraceRotation;
	PC->GetPlayerViewPoint(TraceStart, TraceRotation);

	FVector TraceEnd = TraceStart + (TraceRotation.Vector() * TraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		AActor* HitActor = HitResult.GetActor();
		
		if (HitActor && HitActor->GetClass()->ImplementsInterface(UCarryable::StaticClass()))
		{
			if (CurrentFocusedActor != HitActor)
			{
				CurrentFocusedActor = HitActor;
			}
			return;
		}
	}
	CurrentFocusedActor = nullptr;
}

void UInteractionComponent::PrimaryInteract()
{
	if (!CurrentFocusedActor) return;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	ICarryable* CarryableTarget = Cast<ICarryable>(CurrentFocusedActor);
	if (CarryableTarget && CarryableTarget->CanCarry(OwnerCharacter))
	{
		if (OwnerCharacter->HasAuthority())
		{
			CarryableTarget->OnPickedUp(OwnerCharacter);

			if (UCarryComponent* CarryComp = OwnerCharacter->FindComponentByClass<UCarryComponent>())
			{
				CarryComp->PickUpBox(CurrentFocusedActor);
			}
		}
		else
		{
			Server_RequestPrimaryInteract(CurrentFocusedActor);
		}
	}
}

bool UInteractionComponent::Server_RequestPrimaryInteract_Validate(AActor* TargetActor)
{
	if (!TargetActor) return false;
	
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return false;

	float DistSq = FVector::DistSquared(OwnerCharacter->GetActorLocation(), TargetActor->GetActorLocation());
	return DistSq <= FMath::Square(TraceDistance + 50.f);
}

void UInteractionComponent::Server_RequestPrimaryInteract_Implementation(AActor* TargetActor)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !TargetActor) return;

	ICarryable* CarryableTarget = Cast<ICarryable>(TargetActor);
	
	if (CarryableTarget && CarryableTarget->CanCarry(OwnerCharacter))
	{
		CarryableTarget->OnPickedUp(OwnerCharacter);

		if (UCarryComponent* CarryComp = OwnerCharacter->FindComponentByClass<UCarryComponent>())
		{
			CarryComp->PickUpBox(TargetActor);
		}
	}
}