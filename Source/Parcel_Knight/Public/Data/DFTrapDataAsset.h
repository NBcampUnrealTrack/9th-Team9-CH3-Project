#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DFTrapDataAsset.generated.h"

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
