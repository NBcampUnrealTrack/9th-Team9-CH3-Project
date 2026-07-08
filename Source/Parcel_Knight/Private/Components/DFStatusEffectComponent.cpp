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
	DOREPLIFETIME(UDFStatusEffectComponent, InputInvertEffectState);
}

void UDFStatusEffectComponent::ApplyMoveSpeedModifier(FGameplayTag EffectTag, float Multiplier, float Duration)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatusEffect] ApplyMoveSpeedModifier failed: owner is null"));
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
		UE_LOG(LogTemp, Warning, TEXT("[StatusEffect] ApplyMoveSpeedModifier failed: owner is not a Character (%s)"), *GetNameSafe(Owner));
		return;
	}

	UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement();
	if (!Movement)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatusEffect] ApplyMoveSpeedModifier failed: CharacterMovement is null (%s)"), *GetNameSafe(OwnerCharacter));
		return;
	}

	const float SpeedBeforeApply = Movement->MaxWalkSpeed;
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

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[StatusEffect] Slow applied on server: Owner=%s Effect=%s BaseSpeed=%.2f Multiplier=%.2f Before=%.2f After=%.2f Duration=%.2f"),
		*GetNameSafe(OwnerCharacter),
		*EffectTag.ToString(),
		MoveSpeedEffectState.BaseMaxWalkSpeed,
		MoveSpeedEffectState.Multiplier,
		SpeedBeforeApply,
		Movement->MaxWalkSpeed,
		SafeDuration
	);

	Client_ApplyMoveSpeedEffectState(MoveSpeedEffectState);

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

void UDFStatusEffectComponent::ApplyInputInvert(FGameplayTag EffectTag, float Duration)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatusEffect] ApplyInputInvert failed: owner is null"));
		return;
	}

	if (!Owner->HasAuthority())
	{
		Server_ApplyInputInvert(EffectTag, Duration);
		return;
	}

	const float SafeDuration = FMath::Max(0.0f, Duration);
	InputInvertEffectState.EffectTag = EffectTag;
	InputInvertEffectState.bIsActive = true;

	Owner->ForceNetUpdate();
	Client_ApplyInputInvertEffectState(InputInvertEffectState);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[StatusEffect] Input invert applied on server: Owner=%s Effect=%s Duration=%.2f"),
		*GetNameSafe(Owner),
		*EffectTag.ToString(),
		SafeDuration
	);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InputInvertEffectTimerHandle);

		if (SafeDuration > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				InputInvertEffectTimerHandle,
				this,
				&UDFStatusEffectComponent::ClearInputInvert_ServerOnly,
				SafeDuration,
				false
			);
		}
	}
}

void UDFStatusEffectComponent::ClearInputInvert()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!Owner->HasAuthority())
	{
		Server_ClearInputInvert();
		return;
	}

	ClearInputInvert_ServerOnly();
}

void UDFStatusEffectComponent::Server_ApplyInputInvert_Implementation(FGameplayTag EffectTag, float Duration)
{
	ApplyInputInvert(EffectTag, Duration);
}

void UDFStatusEffectComponent::Server_ClearInputInvert_Implementation()
{
	ClearInputInvert();
}

void UDFStatusEffectComponent::Client_ApplyMoveSpeedEffectState_Implementation(FDFMoveSpeedEffectState NewState)
{
	MoveSpeedEffectState = NewState;
	ApplyMoveSpeedState();
}

void UDFStatusEffectComponent::Client_ApplyInputInvertEffectState_Implementation(FDFInputInvertEffectState NewState)
{
	InputInvertEffectState = NewState;
	OnRep_InputInvertEffectState();
}

void UDFStatusEffectComponent::OnRep_MoveSpeedEffectState()
{
	ApplyMoveSpeedState();
}

void UDFStatusEffectComponent::OnRep_InputInvertEffectState()
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[StatusEffect] Input invert state applied: Owner=%s Authority=%d Active=%d Effect=%s"),
		*GetNameSafe(GetOwner()),
		GetOwner() ? GetOwner()->HasAuthority() : false,
		InputInvertEffectState.bIsActive,
		*InputInvertEffectState.EffectTag.ToString()
	);
}

void UDFStatusEffectComponent::ApplyMoveSpeedState()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ACharacter>(GetOwner());
	}

	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatusEffect] ApplyMoveSpeedState failed: owner is not a Character (%s)"), *GetNameSafe(GetOwner()));
		return;
	}

	UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement();
	if (!Movement)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatusEffect] ApplyMoveSpeedState failed: CharacterMovement is null (%s)"), *GetNameSafe(OwnerCharacter));
		return;
	}

	const float PreviousSpeed = Movement->MaxWalkSpeed;
	if (MoveSpeedEffectState.bIsActive)
	{
		Movement->MaxWalkSpeed = MoveSpeedEffectState.BaseMaxWalkSpeed * MoveSpeedEffectState.Multiplier;
	}
	else if (MoveSpeedEffectState.BaseMaxWalkSpeed > 0.0f)
	{
		Movement->MaxWalkSpeed = MoveSpeedEffectState.BaseMaxWalkSpeed;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[StatusEffect] MoveSpeed state applied: Owner=%s Authority=%d Active=%d Before=%.2f After=%.2f Base=%.2f Multiplier=%.2f"),
		*GetNameSafe(OwnerCharacter),
		OwnerCharacter->HasAuthority(),
		MoveSpeedEffectState.bIsActive,
		PreviousSpeed,
		Movement->MaxWalkSpeed,
		MoveSpeedEffectState.BaseMaxWalkSpeed,
		MoveSpeedEffectState.Multiplier
	);
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
			const float PreviousSpeed = Movement->MaxWalkSpeed;
			Movement->MaxWalkSpeed = MoveSpeedEffectState.BaseMaxWalkSpeed;
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[StatusEffect] Slow expired on server: Owner=%s Before=%.2f Restored=%.2f"),
				*GetNameSafe(OwnerCharacter),
				PreviousSpeed,
				Movement->MaxWalkSpeed
			);
		}
	}

	MoveSpeedEffectState.EffectTag = FGameplayTag();
	MoveSpeedEffectState.Multiplier = 1.0f;
	MoveSpeedEffectState.bIsActive = false;
	Owner->ForceNetUpdate();
	Client_ApplyMoveSpeedEffectState(MoveSpeedEffectState);
}

void UDFStatusEffectComponent::ClearInputInvert_ServerOnly()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InputInvertEffectTimerHandle);
	}

	if (!InputInvertEffectState.bIsActive)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[StatusEffect] Input invert cleared on server: Owner=%s Effect=%s"),
		*GetNameSafe(Owner),
		*InputInvertEffectState.EffectTag.ToString()
	);

	InputInvertEffectState.EffectTag = FGameplayTag();
	InputInvertEffectState.bIsActive = false;
	Owner->ForceNetUpdate();
	Client_ApplyInputInvertEffectState(InputInvertEffectState);
}
