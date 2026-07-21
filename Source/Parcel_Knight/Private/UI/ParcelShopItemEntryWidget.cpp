#include "UI/ParcelShopItemEntryWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/ParcelShopInventoryWidget.h"

void UParcelShopItemEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PurchaseButton)
	{
		PurchaseButton->OnClicked.RemoveDynamic(this, &UParcelShopItemEntryWidget::HandlePurchaseClicked);
		PurchaseButton->OnClicked.AddDynamic(this, &UParcelShopItemEntryWidget::HandlePurchaseClicked);
	}
}

void UParcelShopItemEntryWidget::NativeDestruct()
{
	if (PurchaseButton)
	{
		PurchaseButton->OnClicked.RemoveDynamic(this, &UParcelShopItemEntryWidget::HandlePurchaseClicked);
	}

	OwnerWidget = nullptr;
	Super::NativeDestruct();
}

void UParcelShopItemEntryWidget::InitializeShopItem(
	FGameplayTag InItemId,
	const FText& InDisplayName,
	TSoftObjectPtr<UTexture2D> InIcon,
	int32 InPrice,
	bool bOwned,
	bool bCanAfford,
	UParcelShopInventoryWidget* InOwnerWidget)
{
	ItemId = InItemId;
	OwnerWidget = InOwnerWidget;

	if (ItemIcon)
	{
		ItemIcon->SetBrushFromTexture(InIcon.LoadSynchronous());
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(InDisplayName);
	}

	if (PriceText)
	{
		PriceText->SetText(FText::AsNumber(InPrice));
	}

	if (PurchaseButton)
	{
		PurchaseButton->SetIsEnabled(!bOwned && bCanAfford);
	}
}

void UParcelShopItemEntryWidget::HandlePurchaseClicked()
{
	if (OwnerWidget && ItemId.IsValid())
	{
		OwnerWidget->RequestPurchase(ItemId);
	}
}
