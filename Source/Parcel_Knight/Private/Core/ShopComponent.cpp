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

	
	// GetAllRows로 전체 행을 꺼낸 뒤 ItemTag 필드로 검색
	FItemData* ItemRow     = nullptr;
	bool       bIsCosmetic = false;

	// DataTable 전체 행에서 ItemTag가 일치하는 행을 찾는 람다
	auto FindByTag = [&ItemTag](TArray<FItemData*>& Rows) -> FItemData*
	{
		for (FItemData* Row : Rows)
			if (Row->ItemTag == ItemTag) return Row;
		return nullptr;
	};

	if (ConsumableDataTable)
	{
		TArray<FItemData*> Rows;
		ConsumableDataTable->GetAllRows<FItemData>(TEXT("BuyItem"), Rows);
		ItemRow = FindByTag(Rows);
	}

	// 소모품 테이블에 없으면 코스메틱 테이블에서 찾기
	if (!ItemRow && CosmeticDataTable)
	{
		TArray<FItemData*> Rows;
		CosmeticDataTable->GetAllRows<FItemData>(TEXT("BuyItem"), Rows);
		ItemRow = FindByTag(Rows);
		if (ItemRow) bIsCosmetic = true;
	}

	// 어느 테이블에도 없는 아이템 — 구매 불가
	if (!ItemRow) return false;

	//중복 구매 방지
	if (bIsCosmetic  && GI->HasOwnedCosmetic(ItemTag))  return false;
	if (!bIsCosmetic && GI->HasOwnedConsumable(ItemTag)) return false;
	
	// 아이템 구매
	if (!GI->SpendMoney(ItemRow->Price)) return false;

	// 소유 목록에 추가 후 세이브
	if (bIsCosmetic)
		GI->AddOwnedCosmetic(ItemTag);
	else
		GI->AddOwnedConsumable(ItemTag);

	return true;
}
