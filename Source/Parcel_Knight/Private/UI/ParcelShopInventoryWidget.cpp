#include "UI/ParcelShopInventoryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Core/ParcelGameInstance.h"
#include "Data/ItemData.h"
#include "Engine/DataTable.h"
#include "UI/ParcelInventoryItemEntryWidget.h"
#include "UI/ParcelShopItemEntryWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogParcelShopUI, Log, All);

void UParcelShopInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UParcelShopInventoryWidget::HandleCloseClicked);
		CloseButton->OnClicked.AddDynamic(this, &UParcelShopInventoryWidget::HandleCloseClicked);
	}

	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UParcelShopInventoryWidget::HandleCloseClicked);
		BackButton->OnClicked.AddDynamic(this, &UParcelShopInventoryWidget::HandleCloseClicked);
	}

}

void UParcelShopInventoryWidget::NativeDestruct()
{
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UParcelShopInventoryWidget::HandleCloseClicked);
	}

	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UParcelShopInventoryWidget::HandleCloseClicked);
	}

	InventoryEntries.Reset();
	Super::NativeDestruct();
}

void UParcelShopInventoryWidget::RefreshShopUI()
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI)
	{
		SetStatusMessage(FText::FromString(TEXT("플레이어 프로필을 불러올 수 없습니다.")), true);
		return;
	}

	if (!ItemTable || ItemTable->GetRowStruct() != FItemData::StaticStruct())
	{
		if (ShopItemGrid)
		{
			ShopItemGrid->ClearChildren();
		}
		if (InventoryItemGrid)
		{
			InventoryItemGrid->ClearChildren();
		}
		InventoryEntries.Reset();
		SetStatusMessage(FText::FromString(TEXT("ItemTable에 FItemData DataTable을 지정해 주세요.")), true);
		UE_LOG(LogParcelShopUI, Error, TEXT("Shop refresh failed: invalid ItemTable=%s"), *GetNameSafe(ItemTable));
		return;
	}

	UE_LOG(
		LogParcelShopUI,
		Log,
		TEXT("[ShopInventory] RefreshShopUI: Owned count: %d, Equipped count: %d"),
		GI->GetOwnedConsumables().Num(),
		GI->GetLoadout().Num());

	GI->ValidateSavedLoadout(ItemTable, MaxLoadoutItems);
	SetStatusMessage(FText::GetEmpty(), false);
	BuildShopEntries();
	BuildInventoryEntries();
	RefreshSummary();
}

void UParcelShopInventoryWidget::RequestPurchase(FGameplayTag ItemId)
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI)
	{
		SetStatusMessage(FText::FromString(TEXT("플레이어 프로필을 불러올 수 없습니다.")), true);
		return;
	}

	FText PurchaseStatus;
	const bool bPurchased = GI->TryPurchaseConsumable(ItemTable, ItemId, PurchaseStatus);
	if (bPurchased)
	{
		UE_LOG(LogParcelShopUI, Log, TEXT("[ShopInventory] Purchase success: %s"), *ItemId.ToString());
		UE_LOG(LogParcelShopUI, Log, TEXT("[ShopInventory] Owned count: %d"), GI->GetOwnedConsumables().Num());

		// Use the same complete refresh path as reopening the shop so every panel
		// observes the newly saved ownership state together.
		RefreshShopUI();
	}

	SetStatusMessage(PurchaseStatus, !bPurchased);
}

void UParcelShopInventoryWidget::RequestToggleLoadoutItem(FGameplayTag ItemId)
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI)
	{
		SetStatusMessage(FText::FromString(TEXT("플레이어 프로필을 불러올 수 없습니다.")), true);
		return;
	}

	if (!FindItemData(ItemId))
	{
		SetStatusMessage(FText::FromString(TEXT("DataTable에 없는 아이템은 선택할 수 없습니다.")), true);
		return;
	}

	FText ToggleStatus;
	const bool bChanged = GI->ToggleLoadoutItem(ItemId, MaxLoadoutItems, ToggleStatus);
	RefreshInventorySelectionState();
	RefreshSummary();

	if (!bChanged)
	{
		SetStatusMessage(ToggleStatus, true);
	}
	else
	{
		SetStatusMessage(FText::GetEmpty(), false);
	}
}

void UParcelShopInventoryWidget::BuildShopEntries()
{
	if (!ShopItemGrid)
	{
		SetStatusMessage(FText::FromString(TEXT("ShopItemGrid가 연결되지 않았습니다.")), true);
		return;
	}

	ShopItemGrid->ClearChildren();
	if (!ShopItemEntryClass || !ItemTable)
	{
		SetStatusMessage(FText::FromString(TEXT("ShopItemEntryClass를 지정해 주세요.")), true);
		return;
	}

	const UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI)
	{
		return;
	}

	TArray<FItemData*> Rows;
	ItemTable->GetAllRows<FItemData>(TEXT("BuildShopEntries"), Rows);
	int32 EntryIndex = 0;
	for (const FItemData* Row : Rows)
	{
		if (!Row || !Row->ItemTag.IsValid())
		{
			continue;
		}

		UParcelShopItemEntryWidget* Entry = CreateWidget<UParcelShopItemEntryWidget>(
			GetOwningPlayer(), ShopItemEntryClass);
		if (!Entry)
		{
			continue;
		}

		const bool bOwned = GI->HasOwnedConsumable(Row->ItemTag);
		const bool bCanAfford = GI->GetMoney() >= Row->Price;
		Entry->InitializeShopItem(
			Row->ItemTag,
			Row->DisplayName,
			Row->Icon,
			Row->Price,
			bOwned,
			bCanAfford,
			this);

		const int32 Columns = FMath::Max(1, ShopGridColumns);
		ShopItemGrid->AddChildToUniformGrid(Entry, EntryIndex / Columns, EntryIndex % Columns);
		++EntryIndex;
	}
}

void UParcelShopInventoryWidget::BuildInventoryEntries()
{
	InventoryEntries.Reset();
	if (!InventoryItemGrid)
	{
		UE_LOG(LogParcelShopUI, Error, TEXT("[ShopInventory] InventoryItemGrid is null"));
		SetStatusMessage(FText::FromString(TEXT("InventoryItemGrid가 연결되지 않았습니다.")), true);
		return;
	}

	InventoryItemGrid->ClearChildren();
	if (!InventoryItemEntryClass)
	{
		UE_LOG(LogParcelShopUI, Error, TEXT("[ShopInventory] InventoryItemEntryClass is null"));
		SetStatusMessage(FText::FromString(TEXT("InventoryItemEntryClass를 지정해 주세요.")), true);
		return;
	}

	const UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI)
	{
		UE_LOG(LogParcelShopUI, Error, TEXT("[ShopInventory] ParcelGameInstance is null while building owned entries"));
		return;
	}

	UE_LOG(LogParcelShopUI, Log, TEXT("[ShopInventory] Owned count: %d"), GI->GetOwnedConsumables().Num());

	int32 EntryIndex = 0;
	for (const FGameplayTag& OwnedItem : GI->GetOwnedConsumables())
	{
		UE_LOG(LogParcelShopUI, Log, TEXT("[ShopInventory] Building owned entry: %s"), *OwnedItem.ToString());

		const FItemData* Row = FindItemData(OwnedItem);
		if (!Row)
		{
			UE_LOG(LogParcelShopUI, Warning, TEXT("[ShopInventory] Missing data row for owned tag: %s"), *OwnedItem.ToString());
			continue;
		}

		UParcelInventoryItemEntryWidget* Entry = CreateWidget<UParcelInventoryItemEntryWidget>(
			GetOwningPlayer(), InventoryItemEntryClass);
		if (!Entry)
		{
			UE_LOG(
				LogParcelShopUI,
				Error,
				TEXT("[ShopInventory] Failed to create owned entry: Item=%s, Class=%s, OwningPlayer=%s"),
				*OwnedItem.ToString(),
				*GetNameSafe(InventoryItemEntryClass),
				*GetNameSafe(GetOwningPlayer()));
			continue;
		}

		Entry->InitializeInventoryItem(
			OwnedItem,
			Row->DisplayName,
			Row->Icon,
			GI->GetLoadout().Contains(OwnedItem),
			this);
		Entry->SetVisibility(ESlateVisibility::Visible);

		const int32 Columns = FMath::Max(1, InventoryGridColumns);
		const int32 RowIndex = EntryIndex / Columns;
		const int32 ColumnIndex = EntryIndex % Columns;
		UUniformGridSlot* GridSlot = InventoryItemGrid->AddChildToUniformGrid(Entry, RowIndex, ColumnIndex);
		if (!GridSlot)
		{
			UE_LOG(LogParcelShopUI, Error, TEXT("[ShopInventory] Failed to add owned entry to grid: %s"), *OwnedItem.ToString());
			continue;
		}

		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
		InventoryEntries.Add(Entry);
		UE_LOG(
			LogParcelShopUI,
			Log,
			TEXT("[ShopInventory] Added owned entry: Item=%s, Row=%d, Column=%d"),
			*OwnedItem.ToString(),
			RowIndex,
			ColumnIndex);
		++EntryIndex;
	}

	UE_LOG(
		LogParcelShopUI,
		Log,
		TEXT("[ShopInventory] Owned entry build complete: Created=%d, GridChildren=%d"),
		InventoryEntries.Num(),
		InventoryItemGrid->GetChildrenCount());
}

void UParcelShopInventoryWidget::RefreshInventorySelectionState()
{
	const UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI)
	{
		return;
	}

	for (UParcelInventoryItemEntryWidget* Entry : InventoryEntries)
	{
		if (Entry)
		{
			Entry->SetSelected(GI->GetLoadout().Contains(Entry->GetItemId()));
		}
	}
}

void UParcelShopInventoryWidget::RefreshSummary()
{
	const UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI)
	{
		return;
	}

	if (MoneyText)
	{
		MoneyText->SetText(FText::AsNumber(GI->GetMoney()));
	}

	if (LoadoutCountText)
	{
		LoadoutCountText->SetText(FText::Format(
			FText::FromString(TEXT("{0} / {1}")),
			FText::AsNumber(GI->GetLoadout().Num()),
			FText::AsNumber(MaxLoadoutItems)));
	}
}

void UParcelShopInventoryWidget::SetStatusMessage(const FText& Message, bool bIsError)
{
	if (!StatusText)
	{
		return;
	}

	StatusText->SetText(Message);
	StatusText->SetColorAndOpacity(bIsError ? FSlateColor(FLinearColor::Red) : FSlateColor(FLinearColor::White));
}

const FItemData* UParcelShopInventoryWidget::FindItemData(FGameplayTag ItemId) const
{
	if (!ItemTable || !ItemId.IsValid())
	{
		return nullptr;
	}

	for (const FName RowName : ItemTable->GetRowNames())
	{
		const FItemData* Row = ItemTable->FindRow<FItemData>(RowName, TEXT("FindItemData"));
		if (Row && Row->ItemTag == ItemId)
		{
			return Row;
		}
	}

	return nullptr;
}

void UParcelShopInventoryWidget::HandleCloseClicked()
{
	if (OnCloseRequested.IsBound())
	{
		OnCloseRequested.Broadcast();
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
