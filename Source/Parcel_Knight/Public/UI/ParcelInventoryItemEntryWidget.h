#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "ParcelInventoryItemEntryWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UParcelShopInventoryWidget;
class UTextBlock;
class UTexture2D;

UCLASS()
class PARCEL_KNIGHT_API UParcelInventoryItemEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeInventoryItem(
		FGameplayTag InItemId,
		const FText& InDisplayName,
		TSoftObjectPtr<UTexture2D> InIcon,
		bool bSelected,
		UParcelShopInventoryWidget* InOwnerWidget);

	UFUNCTION(BlueprintCallable, Category = "Parcel|Inventory")
	void SetSelected(bool bSelected);

	FGameplayTag GetItemId() const { return ItemId; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> SelectionBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ItemButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

private:
	UFUNCTION()
	void HandleItemClicked();

	FGameplayTag ItemId;

	UPROPERTY(Transient)
	TObjectPtr<UParcelShopInventoryWidget> OwnerWidget;
};
