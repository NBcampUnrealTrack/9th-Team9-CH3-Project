#include "UI/ParcelInventoryItemEntryWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/ParcelShopInventoryWidget.h"

void UParcelInventoryItemEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ItemButton)
	{
		ItemButton->OnClicked.RemoveDynamic(this, &UParcelInventoryItemEntryWidget::HandleItemClicked);
		ItemButton->OnClicked.AddDynamic(this, &UParcelInventoryItemEntryWidget::HandleItemClicked);
	}
}

void UParcelInventoryItemEntryWidget::NativeDestruct()
{
	if (ItemButton)
	{
		ItemButton->OnClicked.RemoveDynamic(this, &UParcelInventoryItemEntryWidget::HandleItemClicked);
	}

	OwnerWidget = nullptr;
	Super::NativeDestruct();
}

void UParcelInventoryItemEntryWidget::InitializeInventoryItem(
	FGameplayTag InItemId,
	const FText& InDisplayName,
	TSoftObjectPtr<UTexture2D> InIcon,
	bool bSelected,
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

	SetSelected(bSelected);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[ShopInventory] Inventory entry initialized: Item=%s, Selected=%s, ItemButtonValid=%s"),
		*ItemId.ToString(),
		bSelected ? TEXT("true") : TEXT("false"),
		ItemButton ? TEXT("true") : TEXT("false"));
}

void UParcelInventoryItemEntryWidget::SetSelected(bool bSelected)
{
	if (!SelectionBorder)
	{
		return;
	}

	FLinearColor BorderColor = FLinearColor::Red;
	BorderColor.A = bSelected ? 1.0f : 0.0f;
	SelectionBorder->SetBrushColor(BorderColor);
	// Ignore hits on the border itself while preserving hit tests for the
	// child ItemButton used to select and deselect this owned item.
	SelectionBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UParcelInventoryItemEntryWidget::HandleItemClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[ShopInventory] Owned item clicked: %s"), *ItemId.ToString());

	if (OwnerWidget && ItemId.IsValid())
	{
		OwnerWidget->RequestToggleLoadoutItem(ItemId);
	}
}
