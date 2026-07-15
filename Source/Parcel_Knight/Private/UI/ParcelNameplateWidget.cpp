#include "UI/ParcelNameplateWidget.h"
#include "Components/TextBlock.h"

void UParcelNameplateWidget::SetPlayerName(const FString& InName)
{
	if (Txt_PlayerName)
	{
		Txt_PlayerName->SetText(FText::FromString(InName));
	}
}

void UParcelNameplateWidget::UpdateStatusEffects(const FGameplayTagContainer& ActiveTags)
{
	K2_OnStatusEffectsChanged(ActiveTags);
	
	if (Txt_StatusEffect)
	{
		TArray<FString> StatusTexts;
		
		if (ActiveTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Ragdoll"))))
		{
			StatusTexts.Add(TEXT("기절"));
		}
		if (ActiveTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Dead"))))
		{
			StatusTexts.Add(TEXT("죽음"));
		}
		if (ActiveTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Exhausted"))))
		{
			StatusTexts.Add(TEXT("지침"));
		}
		if (ActiveTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Carrying"))))
		{
			StatusTexts.Add(TEXT("운반중"));
		}
		
		// Todo : 함정 State 랑 연동지어서 Reverse 상태도 Tag로 연동할 필요.

		if (StatusTexts.Num() > 0)
		{
			FString FinalStr = TEXT("[") + FString::Join(StatusTexts, TEXT(", ")) + TEXT("]");
			Txt_StatusEffect->SetText(FText::FromString(FinalStr));
			Txt_StatusEffect->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Txt_StatusEffect->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}