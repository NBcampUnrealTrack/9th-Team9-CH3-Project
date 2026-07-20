#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DFTrapDataAsset.generated.h"

class UDamageType;
class UParticleSystem;
class USoundAttenuation;
class USoundBase;

UCLASS(BlueprintType)
class PARCEL_KNIGHT_API UDFTrapDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UDFTrapDataAsset();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Tags")
	FGameplayTag TrapTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Tags")
	FGameplayTag TriggerTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Tags")
	FGameplayTag EffectTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Timing", meta = (ClampMin = "0.0"))
	float ActivationDelay = 0.0f;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use ActivationDelay. Existing overlap traps activate immediately by default."))
	float WarningTime_DEPRECATED = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Timing", meta = (ClampMin = "0.0"))
	float ActiveDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Timing", meta = (ClampMin = "0.0"))
	float Cooldown = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effect", meta = (ClampMin = "0.0"))
	float EffectDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effect", meta = (ClampMin = "0.0"))
	float EffectMagnitude = 0.5f;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use DamageAmount in the Trap|Damage category."))
	float TrapDamage_DEPRECATED = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Damage")
	bool bApplyDamageOnOverlap = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Damage", meta = (ClampMin = "0.0"))
	float DamageAmount = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Damage|Held Box", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float HeldBoxDamageMultiplier = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Damage", meta = (EditCondition = "bApplyDamageOnOverlap"))
	TSubclassOf<UDamageType> DamageTypeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Damage", meta = (EditCondition = "bApplyDamageOnOverlap"))
	bool bDamageOnlyOncePerActivation = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Damage", meta = (EditCondition = "bApplyDamageOnOverlap && !bDamageOnlyOncePerActivation", ClampMin = "0.0"))
	float DamageCooldownPerActor = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effect", meta = (ClampMin = "0.0"))
	float PushStrength = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effect", meta = (ClampMin = "0.0"))
	float PushUpStrength = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effect", meta = (ClampMin = "0.0"))
	float MaxPushSpeed = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effect")
	bool bUseTrapForwardAsPushDirection = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effect")
	bool bUseOppositeVelocityForReverse = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effect", meta = (ClampMin = "0.01"))
	float RepeatInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effect")
	bool bRepeatWhileOverlapping = false;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Persistent effects now expire only through EffectDuration."))
	bool bRemoveEffectOnEndOverlap_DEPRECATED = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Behavior")
	bool bTriggerOnce = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Target")
	bool bAffectsPlayer = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Target")
	bool bAffectsCarriedBox = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|FX")
	TObjectPtr<UParticleSystem> ActivateVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Feedback|Audio")
	TObjectPtr<USoundBase> ActivationSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Feedback|Audio", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ActivationSoundVolume = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Feedback|Audio", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float ActivationSoundPitch = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Feedback|Audio")
	TObjectPtr<USoundAttenuation> ActivationSoundAttenuation;

	// Legacy field kept for serialized compatibility. Common activation playback uses ActivationSound.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|FX")
	TObjectPtr<USoundBase> ActivateSFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|FX")
	TObjectPtr<UParticleSystem> ResetVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|FX")
	TObjectPtr<USoundBase> ResetSFX;
};
