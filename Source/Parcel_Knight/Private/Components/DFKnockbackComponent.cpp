#include "Components/DFKnockbackComponent.h"

#include "GameFramework/Character.h"

UDFKnockbackComponent::UDFKnockbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UDFKnockbackComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
}

void UDFKnockbackComponent::ApplyKnockbackFromLocation(FVector SourceLocation, float Strength, float UpwardStrength)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!Owner->HasAuthority())
	{
		Server_ApplyKnockbackFromLocation(SourceLocation, Strength, UpwardStrength);
		return;
	}

	ACharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return;
	}

	FVector Direction = Character->GetActorLocation() - SourceLocation;
	ApplyKnockback(Direction, Strength, UpwardStrength);
}

void UDFKnockbackComponent::ApplyKnockback(FVector Direction, float Strength, float UpwardStrength)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!Owner->HasAuthority())
	{
		Server_ApplyKnockback(Direction, Strength, UpwardStrength);
		return;
	}

	ACharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return;
	}

	Direction.Z = 0.0f;
	if (!Direction.Normalize())
	{
		return;
	}

	const FVector LaunchVelocity = Direction * Strength + FVector(0.0f, 0.0f, UpwardStrength);
	Character->LaunchCharacter(LaunchVelocity, true, true);
	Character->ForceNetUpdate();
}

void UDFKnockbackComponent::Server_ApplyKnockbackFromLocation_Implementation(
	FVector SourceLocation,
	float Strength,
	float UpwardStrength
)
{
	ApplyKnockbackFromLocation(SourceLocation, Strength, UpwardStrength);
}

void UDFKnockbackComponent::Server_ApplyKnockback_Implementation(
	FVector Direction,
	float Strength,
	float UpwardStrength
)
{
	ApplyKnockback(Direction, Strength, UpwardStrength);
}

ACharacter* UDFKnockbackComponent::GetOwnerCharacter()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ACharacter>(GetOwner());
	}

	return OwnerCharacter;
}
