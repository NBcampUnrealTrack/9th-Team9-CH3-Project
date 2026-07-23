#include "Components/DFKnockbackComponent.h"

#include "Character/RagdollComponent.h"
#include "Core/HealthComponent.h"
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
		UE_LOG(LogTemp, Verbose, TEXT("[Knockback] Client numeric request rejected for %s"), *GetNameSafe(Owner));
		return;
	}

	if (!FMath::IsFinite(SourceLocation.X)
		|| !FMath::IsFinite(SourceLocation.Y)
		|| !FMath::IsFinite(SourceLocation.Z)
		|| !FMath::IsFinite(Strength)
		|| !FMath::IsFinite(UpwardStrength)
		|| Strength < 0.0f
		|| UpwardStrength < 0.0f)
	{
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
		UE_LOG(LogTemp, Verbose, TEXT("[Knockback] Client numeric request rejected for %s"), *GetNameSafe(Owner));
		return;
	}

	if (!FMath::IsFinite(Direction.X)
		|| !FMath::IsFinite(Direction.Y)
		|| !FMath::IsFinite(Direction.Z)
		|| !FMath::IsFinite(Strength)
		|| !FMath::IsFinite(UpwardStrength)
		|| Strength < 0.0f
		|| UpwardStrength < 0.0f)
	{
		return;
	}

	ACharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return;
	}

	if (const UHealthComponent* HealthComponent = Character->FindComponentByClass<UHealthComponent>())
	{
		if (HealthComponent->IsDead())
		{
			return;
		}
	}

	if (const URagdollComponent* RagdollComponent = Character->FindComponentByClass<URagdollComponent>())
	{
		if (RagdollComponent->IsRagdoll())
		{
			return;
		}
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
	UE_LOG(LogTemp, Verbose, TEXT("[Knockback] Legacy client location request rejected for %s"), *GetNameSafe(GetOwner()));
}

void UDFKnockbackComponent::Server_ApplyKnockback_Implementation(
	FVector Direction,
	float Strength,
	float UpwardStrength
)
{
	UE_LOG(LogTemp, Verbose, TEXT("[Knockback] Legacy client direction request rejected for %s"), *GetNameSafe(GetOwner()));
}

ACharacter* UDFKnockbackComponent::GetOwnerCharacter()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ACharacter>(GetOwner());
	}

	return OwnerCharacter;
}
