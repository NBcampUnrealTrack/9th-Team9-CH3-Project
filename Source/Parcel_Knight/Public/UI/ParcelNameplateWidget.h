#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Data/ItemData.h"
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

	// 칭호 설정 — nullptr이면 칭호 숨김
	void SetTitle(const FItemData* TitleData);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_PlayerName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_StatusEffect;

	// 닉네임 위에 표시되는 칭호 텍스트 — 블루프린트 위젯에 같은 이름으로 추가 필요
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Title;

	UFUNCTION(BlueprintImplementableEvent, Category = "ParcelUI")
	void K2_OnStatusEffectsChanged(const FGameplayTagContainer& ActiveTags);
};