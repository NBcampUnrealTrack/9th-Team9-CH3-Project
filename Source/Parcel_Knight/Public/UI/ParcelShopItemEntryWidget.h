#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "ParcelShopItemEntryWidget.generated.h"

class UButton;
class UImage;
class UParcelShopInventoryWidget;
class UTextBlock;
class UTexture2D;

UCLASS()
class PARCEL_KNIGHT_API UParcelShopItemEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeShopItem(
		FGameplayTag InItemId,
		const FText& InDisplayName,
		TSoftObjectPtr<UTexture2D> InIcon,
		int32 InPrice,
		bool bOwned,
		bool bCanAfford,
		UParcelShopInventoryWidget* InOwnerWidget);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PriceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PurchaseButton;

private:
	UFUNCTION()
	void HandlePurchaseClicked();

	FGameplayTag ItemId;

	UPROPERTY(Transient)
	TObjectPtr<UParcelShopInventoryWidget> OwnerWidget;
};
