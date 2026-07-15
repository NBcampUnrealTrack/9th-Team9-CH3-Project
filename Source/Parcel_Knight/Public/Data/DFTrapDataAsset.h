#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DFTrapDataAsset.generated.h"

class UDamageType;
class UParticleSystem;
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
	float WarningTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Timing", meta = (ClampMin = "0.0"))
	float ActiveDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Timing", meta = (ClampMin = "0.0"))
	float Cooldown = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effect", meta = (ClampMin = "0.0"))
	float EffectDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effect", meta = (ClampMin = "0.0"))
	float EffectMagnitude = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effect", meta = (ClampMin = "0.0"))
	float TrapDamage = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Damage")
	bool bApplyDamageOnOverlap = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Damage", meta = (EditCondition = "bApplyDamageOnOverlap", ClampMin = "0.0"))
	float DamageAmount = 0.0f;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Effect")
	bool bRemoveEffectOnEndOverlap = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Behavior")
	bool bTriggerOnce = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Target")
	bool bAffectsPlayer = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|Target")
	bool bAffectsCarriedBox = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|FX")
	TObjectPtr<UParticleSystem> ActivateVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|FX")
	TObjectPtr<USoundBase> ActivateSFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|FX")
	TObjectPtr<UParticleSystem> ResetVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trap|FX")
	TObjectPtr<USoundBase> ResetSFX;
};
