#include "Core/InventoryComponent.h"
#include "Core/ParcelGameInstance.h"
#include "Data/ItemData.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInventoryComponent, Items);
}

// ========================= 초기화 =========================

void UInventoryComponent::InitFromGameInstance(UParcelGameInstance* GI)
{
	if (!GI) return;

	Items = GI->GetLoadout();

	// 비어있는 슬롯(EmptyTag) 제거
	Items.RemoveAll([](const FGameplayTag& Tag) { return !Tag.IsValid(); });
}

// ========================= 조회 =========================

bool UInventoryComponent::HasItem(FGameplayTag ItemTag) const
{
	return Items.Contains(ItemTag);
}

const TArray<FGameplayTag>& UInventoryComponent::GetItems() const
{
	return Items;
}

// ========================= 사용 =========================

bool UInventoryComponent::UseItem(FGameplayTag ItemTag)
{
	// 서버에서만 인벤토리 변경 — 결과는 Items 복제로 클라이언트에 전달됨
	if (!GetOwner()->HasAuthority()) return false;
	if (!HasItem(ItemTag)) return false;

	if (ConsumableDataTable)
	{
		// GetAllRows로 전체 행을 꺼낸 뒤 ItemTag 필드로 검색
		TArray<FItemData*> AllRows;
		ConsumableDataTable->GetAllRows<FItemData>(TEXT("UseItem"), AllRows);

		for (FItemData* Row : AllRows)
		{
			if (Row->ItemTag == ItemTag)
			{
				// 영구 아이템이면 제거하지 않고 성공 반환
				if (Row->bIsPermanent) return true;
				break;
			}
		}
	}

	Items.Remove(ItemTag);
	return true;
}

// ========================= 복제 콜백 =========================

void UInventoryComponent::OnRep_Items()
{
	// 클라이언트에서 Items가 갱신되면 UI에 알림
	OnInventoryChanged.Broadcast();
}
