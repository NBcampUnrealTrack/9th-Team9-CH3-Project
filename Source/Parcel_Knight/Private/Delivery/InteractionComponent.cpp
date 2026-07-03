#include "Delivery/InteractionComponent.h"
#include "Delivery/InteractableInterface.h"
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

	AActor* NewFocus = nullptr;

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		AActor* HitActor = HitResult.GetActor();
		
		if (HitActor && HitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
		{
			NewFocus = HitActor;
		}
	}

	if (CurrentFocusedActor != NewFocus)
	{
		CurrentFocusedActor = NewFocus;
		OnFocusChanged.Broadcast(CurrentFocusedActor);
	}
}

void UInteractionComponent::PrimaryInteract()
{
	if (!CurrentFocusedActor) return;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	if (CurrentFocusedActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		if (IInteractableInterface::Execute_CanInteract(CurrentFocusedActor, OwnerCharacter))
		{
			if (OwnerCharacter->HasAuthority())
			{
				// Host 유저 : 즉시 상자 상호작용 호출
				IInteractableInterface::Execute_Interact(CurrentFocusedActor, OwnerCharacter);
			}
			else
			{
				// 원격 클라이언트 유저 : RPC 요청 발송
				Server_RequestPrimaryInteract(CurrentFocusedActor);
			}
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
	if (TargetActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		if (IInteractableInterface::Execute_CanInteract(TargetActor, OwnerCharacter))
		{
			IInteractableInterface::Execute_Interact(TargetActor, OwnerCharacter);
		}
	}
}