#include "Components/DFStatusEffectComponent.h"

#include "Character/ParcelMovementStatComponent.h"
#include "Core/HealthComponent.h"
#include "GameFramework/Character.h"
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
		UE_LOG(LogTemp, Verbose, TEXT("[StatusEffect] Client move-speed request rejected for %s"), *GetNameSafe(Owner));
		return;
	}

	if (!EffectTag.IsValid()
		|| !FMath::IsFinite(Multiplier)
		|| !FMath::IsFinite(Duration)
		|| Multiplier < 0.0f
		|| Duration <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatusEffect] Invalid server move-speed configuration rejected for %s"), *GetNameSafe(Owner));
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

	if (const UHealthComponent* HealthComponent = OwnerCharacter->FindComponentByClass<UHealthComponent>())
	{
		if (HealthComponent->IsDead())
		{
			return;
		}
	}

	UParcelMovementStatComponent* MovementStat = OwnerCharacter->FindComponentByClass<UParcelMovementStatComponent>();
	if (!MovementStat)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatusEffect] ApplyMoveSpeedModifier failed: MovementStatComponent is null (%s)"), *GetNameSafe(OwnerCharacter));
		return;
	}

	const float SpeedBeforeApply = MovementStat->GetMaxWalkSpeed();
	const float SafeMultiplier = FMath::Max(0.0f, Multiplier);
	const float SafeDuration = FMath::Max(0.0f, Duration);
	const float LegacyBaseSpeed = MoveSpeedEffectState.bIsActive
		? MoveSpeedEffectState.BaseMaxWalkSpeed
		: SpeedBeforeApply;

	MoveSpeedEffectState.EffectTag = EffectTag;
	MoveSpeedEffectState.Multiplier = SafeMultiplier;
	// 공개 구조체의 기존 관찰 값은 유지하되, 최종 속도 계산이나 복원에는 사용하지 않는다.
	MoveSpeedEffectState.BaseMaxWalkSpeed = LegacyBaseSpeed;
	MoveSpeedEffectState.bIsActive = true;

	ApplyMoveSpeedState();
	Owner->ForceNetUpdate();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[StatusEffect] Slow applied on server: Owner=%s Effect=%s Multiplier=%.2f Before=%.2f After=%.2f Duration=%.2f"),
		*GetNameSafe(OwnerCharacter),
		*EffectTag.ToString(),
		MoveSpeedEffectState.Multiplier,
		SpeedBeforeApply,
		MovementStat->GetMaxWalkSpeed(),
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
	UE_LOG(LogTemp, Verbose, TEXT("[StatusEffect] Legacy client move-speed RPC rejected for %s"), *GetNameSafe(GetOwner()));
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
		UE_LOG(LogTemp, Verbose, TEXT("[StatusEffect] Client input-invert request rejected for %s"), *GetNameSafe(Owner));
		return;
	}

	if (!EffectTag.IsValid() || !FMath::IsFinite(Duration) || Duration <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatusEffect] Invalid server input-invert configuration rejected for %s"), *GetNameSafe(Owner));
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

	if (const UHealthComponent* HealthComponent = OwnerCharacter->FindComponentByClass<UHealthComponent>())
	{
		if (HealthComponent->IsDead())
		{
			return;
		}
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
		UE_LOG(LogTemp, Verbose, TEXT("[StatusEffect] Client clear request rejected for %s"), *GetNameSafe(Owner));
		return;
	}

	ClearInputInvert_ServerOnly();
}

void UDFStatusEffectComponent::Server_ApplyInputInvert_Implementation(FGameplayTag EffectTag, float Duration)
{
	UE_LOG(LogTemp, Verbose, TEXT("[StatusEffect] Legacy client input-invert RPC rejected for %s"), *GetNameSafe(GetOwner()));
}

void UDFStatusEffectComponent::Server_ClearInputInvert_Implementation()
{
	UE_LOG(LogTemp, Verbose, TEXT("[StatusEffect] Legacy client clear RPC rejected for %s"), *GetNameSafe(GetOwner()));
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
	// Client RPC와 RepNotify가 모두 도착해도 동일한 최종 속도를 재적용하므로 안전하다.
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

	UParcelMovementStatComponent* MovementStat = OwnerCharacter->FindComponentByClass<UParcelMovementStatComponent>();
	if (!MovementStat)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatusEffect] ApplyMoveSpeedState failed: MovementStatComponent is null (%s)"), *GetNameSafe(OwnerCharacter));
		return;
	}

	const float PreviousSpeed = MovementStat->GetMaxWalkSpeed();
	MovementStat->RefreshMoveSpeed();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[StatusEffect] MoveSpeed state applied: Owner=%s Authority=%d Active=%d Before=%.2f After=%.2f Multiplier=%.2f"),
		*GetNameSafe(OwnerCharacter),
		OwnerCharacter->HasAuthority(),
		MoveSpeedEffectState.bIsActive,
		PreviousSpeed,
		MovementStat->GetMaxWalkSpeed(),
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

	if (!MoveSpeedEffectState.bIsActive)
	{
		return;
	}

	UParcelMovementStatComponent* MovementStat = OwnerCharacter
		? OwnerCharacter->FindComponentByClass<UParcelMovementStatComponent>()
		: nullptr;
	const float PreviousSpeed = MovementStat ? MovementStat->GetMaxWalkSpeed() : 0.0f;

	MoveSpeedEffectState.EffectTag = FGameplayTag();
	MoveSpeedEffectState.Multiplier = 1.0f;
	MoveSpeedEffectState.bIsActive = false;
	ApplyMoveSpeedState();
	Owner->ForceNetUpdate();
	Client_ApplyMoveSpeedEffectState(MoveSpeedEffectState);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[StatusEffect] Slow expired on server: Owner=%s Before=%.2f Recalculated=%.2f"),
		*GetNameSafe(OwnerCharacter),
		PreviousSpeed,
		MovementStat ? MovementStat->GetMaxWalkSpeed() : 0.0f
	);
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
