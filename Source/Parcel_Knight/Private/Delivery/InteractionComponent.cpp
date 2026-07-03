#include "Delivery/InteractionComponent.h"
#include "Delivery/Interactable.h"
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
		
		if (HitActor && HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
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

	IInteractable* InteractableTarget = Cast<IInteractable>(CurrentFocusedActor);
	if (InteractableTarget && InteractableTarget->CanInteract(OwnerCharacter))
	{
		if (OwnerCharacter->HasAuthority())
		{
			// Host 유저 : 즉시 상자 상호작용 호출
			InteractableTarget->Interact(OwnerCharacter);
		}
		else
		{
			// 원격 클라이언트 유저 : RPC 요청 발송
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
	
	// [Server] 서버에서도 상자의 IInteractable 인지
	IInteractable* InteractableTarget = Cast<IInteractable>(TargetActor);
	if (InteractableTarget && InteractableTarget->CanInteract(OwnerCharacter))
	{
		InteractableTarget->Interact(OwnerCharacter);
	}
}