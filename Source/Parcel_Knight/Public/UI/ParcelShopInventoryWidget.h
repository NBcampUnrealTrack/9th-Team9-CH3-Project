#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "ParcelShopInventoryWidget.generated.h"

class UButton;
class UDataTable;
class UParcelInventoryItemEntryWidget;
class UParcelShopItemEntryWidget;
class UTextBlock;
class UUniformGridPanel;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnParcelShopCloseRequested);

/** Frontend shop and persistent three-item loadout screen. */
UCLASS()
class PARCEL_KNIGHT_API UParcelShopInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Parcel|Shop")
	void RefreshShopUI();

	void RequestPurchase(FGameplayTag ItemId);
	void RequestToggleLoadoutItem(FGameplayTag ItemId);

	UPROPERTY(BlueprintAssignable, Category = "Parcel|Shop")
	FOnParcelShopCloseRequested OnCloseRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	UPROPERTY(EditDefaultsOnly, Category = "MainMenu|Sound")
	TObjectPtr<USoundBase> ButtonClickSound;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> ShopItemGrid;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> InventoryItemGrid;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MoneyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LoadoutCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parcel|Shop|Data")
	TObjectPtr<UDataTable> ItemTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parcel|Shop|Entries")
	TSubclassOf<UParcelShopItemEntryWidget> ShopItemEntryClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parcel|Shop|Entries")
	TSubclassOf<UParcelInventoryItemEntryWidget> InventoryItemEntryClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parcel|Shop", meta = (ClampMin = "1", ClampMax = "3"))
	int32 MaxLoadoutItems = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parcel|Shop|Layout", meta = (ClampMin = "1"))
	int32 ShopGridColumns = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parcel|Shop|Layout", meta = (ClampMin = "1"))
	int32 InventoryGridColumns = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parcel|Shop|Layout", meta = (ClampMin = "1"))
	int32 ShopSlotCount = 9;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parcel|Shop|Layout", meta = (ClampMin = "1"))
	int32 InventorySlotCount = 9;

private:
	void PlayButtonClickSound();

	UFUNCTION()
	void HandleCloseClicked();

	void BuildShopSlots();
	void BuildInventorySlots();
	void BuildShopEntries();
	void BuildInventoryEntries();
	void RefreshInventorySelectionState();
	void RefreshSummary();
	void SetStatusMessage(const FText& Message, bool bIsError);
	const struct FItemData* FindItemData(FGameplayTag ItemId) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UParcelShopItemEntryWidget>> ShopEntries;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UParcelInventoryItemEntryWidget>> InventoryEntries;
};
