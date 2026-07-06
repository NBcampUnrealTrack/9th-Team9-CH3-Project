#include "Components/DFStatusEffectComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UDFStatusEffectComponent::UDFStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UDFStatusEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
}

void UDFStatusEffectComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UDFStatusEffectComponent, MoveSpeedEffectState);
}

void UDFStatusEffectComponent::ApplyMoveSpeedModifier(FGameplayTag EffectTag, float Multiplier, float Duration)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!Owner->HasAuthority())
	{
		Server_ApplyMoveSpeedModifier(EffectTag, Multiplier, Duration);
		return;
	}

	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ACharacter>(Owner);
	}

	if (!OwnerCharacter)
	{
		return;
	}

	UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	const float SafeMultiplier = FMath::Max(0.0f, Multiplier);
	const float SafeDuration = FMath::Max(0.0f, Duration);
	const float BaseSpeed = MoveSpeedEffectState.bIsActive
		? MoveSpeedEffectState.BaseMaxWalkSpeed
		: Movement->MaxWalkSpeed;

	MoveSpeedEffectState.EffectTag = EffectTag;
	MoveSpeedEffectState.Multiplier = SafeMultiplier;
	MoveSpeedEffectState.BaseMaxWalkSpeed = BaseSpeed;
	MoveSpeedEffectState.bIsActive = true;

	ApplyMoveSpeedState();
	Owner->ForceNetUpdate();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MoveSpeedEffectTimerHandle);

		if (SafeDuration > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				MoveSpeedEffectTimerHandle,
				this,
				&UDFStatusEffectComponent::ClearMoveSpeedModifier_ServerOnly,
				SafeDuration,
				false
			);
		}
	}
}

void UDFStatusEffectComponent::Server_ApplyMoveSpeedModifier_Implementation(
	FGameplayTag EffectTag,
	float Multiplier,
	float Duration
)
{
	ApplyMoveSpeedModifier(EffectTag, Multiplier, Duration);
}

void UDFStatusEffectComponent::OnRep_MoveSpeedEffectState()
{
	ApplyMoveSpeedState();
}

void UDFStatusEffectComponent::ApplyMoveSpeedState()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ACharacter>(GetOwner());
	}

	if (!OwnerCharacter)
	{
		return;
	}

	UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	if (MoveSpeedEffectState.bIsActive)
	{
		Movement->MaxWalkSpeed = MoveSpeedEffectState.BaseMaxWalkSpeed * MoveSpeedEffectState.Multiplier;
	}
	else if (MoveSpeedEffectState.BaseMaxWalkSpeed > 0.0f)
	{
		Movement->MaxWalkSpeed = MoveSpeedEffectState.BaseMaxWalkSpeed;
	}
}

void UDFStatusEffectComponent::ClearMoveSpeedModifier_ServerOnly()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ACharacter>(Owner);
	}

	if (OwnerCharacter)
	{
		if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = MoveSpeedEffectState.BaseMaxWalkSpeed;
		}
	}

	MoveSpeedEffectState.EffectTag = FGameplayTag();
	MoveSpeedEffectState.Multiplier = 1.0f;
	MoveSpeedEffectState.bIsActive = false;
	Owner->ForceNetUpdate();
}
