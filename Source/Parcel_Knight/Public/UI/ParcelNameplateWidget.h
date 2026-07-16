#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "ParcelNameplateWidget.generated.h"

class UTextBlock;

/**
 *
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelNameplateWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPlayerName(const FString& InName);

	void UpdateStatusEffects(const FGameplayTagContainer& ActiveTags);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_PlayerName;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_StatusEffect;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "ParcelUI")
	void K2_OnStatusEffectsChanged(const FGameplayTagContainer& ActiveTags);
};