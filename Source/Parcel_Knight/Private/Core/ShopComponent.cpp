#include "Core/ShopComponent.h"
#include "Core/ParcelGameInstance.h"
#include "Data/ItemData.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"

// ========================= 조회 =========================

TArray<FItemData*> UShopComponent::GetConsumableItems() const
{
	TArray<FItemData*> OutRows;
	if (ConsumableDataTable)
		ConsumableDataTable->GetAllRows<FItemData>(TEXT("GetConsumableItems"), OutRows);
	return OutRows;
}

TArray<FItemData*> UShopComponent::GetCosmeticItems() const
{
	TArray<FItemData*> OutRows;
	if (CosmeticDataTable)
		CosmeticDataTable->GetAllRows<FItemData>(TEXT("GetCosmeticItems"), OutRows);
	return OutRows;
}

// ========================= 구매 =========================

bool UShopComponent::BuyItem(APlayerController* Buyer, FGameplayTag ItemTag)
{
	// 서버에서만 구매 처리 — 클라이언트가 직접 호출 불가
	if (!GetOwner()->HasAuthority()) return false;

	if (!Buyer) return false;

	UParcelGameInstance* GI = Buyer->GetGameInstance<UParcelGameInstance>();
	if (!GI) return false;

	// Consumable purchases use the same validated, atomic profile transaction as the frontend shop.
	FText PurchaseStatus;
	if (GI->TryPurchaseConsumable(ConsumableDataTable, ItemTag, PurchaseStatus))
	{
		return true;
	}

	// Cosmetic purchases retain their existing path.
	FItemData* ItemRow = nullptr;

	// DataTable 전체 행에서 ItemTag가 일치하는 행을 찾는 람다
	auto FindByTag = [&ItemTag](TArray<FItemData*>& Rows) -> FItemData*
	{
		for (FItemData* Row : Rows)
			if (Row->ItemTag == ItemTag) return Row;
		return nullptr;
	};

	if (CosmeticDataTable)
	{
		TArray<FItemData*> Rows;
		CosmeticDataTable->GetAllRows<FItemData>(TEXT("BuyItem"), Rows);
		ItemRow = FindByTag(Rows);
	}

	// 어느 테이블에도 없는 아이템 — 구매 불가
	if (!ItemRow) return false;

	//중복 구매 방지
	if (GI->HasOwnedCosmetic(ItemTag)) return false;
	
	// 아이템 구매
	if (!GI->SpendMoney(ItemRow->Price)) return false;

	// 소유 목록에 추가 후 세이브
	GI->AddOwnedCosmetic(ItemTag);

	return true;
}
