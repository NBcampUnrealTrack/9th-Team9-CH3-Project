// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/CharacterCarryComponent.h"
#include "Components/ActorComponent.h" 
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Delivery/DeliveryBox.h"
#include "Delivery/Carryable.h"
#include "Delivery/PhysicsJudgeManager.h"

UCharacterCarryComponent::UCharacterCarryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CarriedBox = nullptr;
	bIsCarrying = false;
	MoveSpeedMultiplier = 1.0f;
	DefaultMaxWalkSpeed = 450.f;
}

void UCharacterCarryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
		{
			DefaultMaxWalkSpeed = Movement->MaxWalkSpeed;
		}
	}
}

void UCharacterCarryComponent::Pickup(ADeliveryBox* InBox)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !InBox) return;

	CarriedBox = InBox;
	bIsCarrying = true;
	MoveSpeedMultiplier = InBox->GetBoxData().MoveSpeedMultiplier;

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
	InBox->AttachToComponent(OwnerCharacter->GetMesh(), AttachmentRules, TEXT("HandSocket"));

	if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = DefaultMaxWalkSpeed * MoveSpeedMultiplier;
	}
}

void UCharacterCarryComponent::Drop()
{
	if (!CarriedBox) return;

	if (GetOwner()->HasAuthority())
	{
		if (ICarryable* Carryable = Cast<ICarryable>(CarriedBox))
		{
			Carryable->OnDropped();
		}
	}

	CarriedBox->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = DefaultMaxWalkSpeed; // 기본 걷기 속도 복구
		}
	}

	CarriedBox = nullptr;
	bIsCarrying = false;
}

void UCharacterCarryComponent::Throw(FVector Force)
{
	if (!CarriedBox) return;

	if (GetOwner()->HasAuthority())
	{
		AActor* DroppedBox = CarriedBox;

		if (ICarryable* Carryable = Cast<ICarryable>(DroppedBox))
		{
			Carryable->OnDropped();
		}

		DroppedBox->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(DroppedBox->GetRootComponent()))
		{
			RootPrim->AddImpulse(Force, NAME_None, true); // true = 질량을 무시하고 속도 변화로 던짐
		}

		ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
		if (OwnerCharacter)
		{
			if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed = DefaultMaxWalkSpeed;
			}
		}

		CarriedBox = nullptr;
		bIsCarrying = false;
	}
	else
	{
		Server_Throw(Force);
	}
}

void UCharacterCarryComponent::ForceDropByTrap(float TrapDamage)
{
	if (!CarriedBox) return;

	ADeliveryBox* BoxActor = CarriedBox;

	Drop();

	if (GetOwner()->HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UPhysicsJudgeManager* JudgeManager = World->GetSubsystem<UPhysicsJudgeManager>())
			{
				JudgeManager->EvaluateTrapImpact(BoxActor, TrapDamage);
			}
		}
	}
}

bool UCharacterCarryComponent::Server_Throw_Validate(FVector Force)
{
	return true;
}

void UCharacterCarryComponent::Server_Throw_Implementation(FVector Force)
{
	Throw(Force);
}
