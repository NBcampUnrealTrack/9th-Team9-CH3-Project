#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ShopComponent.generated.h"

class UDataTable;
struct FItemData;

/**
 * 상점 아이템 목록 제공 및 구매 처리를 담당하는 컴포넌트
 * GameState에 부착되며 모든 플레이어가 공유한다.
 * 구매 여부(구매됨 표시)는 UI에서 GameInstance.OwnedConsumables/OwnedCosmetics와 비교해 판단한다.
 *
 * 담당자: 한수현
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UShopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// ========================= 조회 =========================

	// [All] 소모품 아이템 목록 반환 — UI 표시용
	TArray<FItemData*> GetConsumableItems() const;

	// [All] 코스메틱 아이템 목록 반환 — UI 표시용
	TArray<FItemData*> GetCosmeticItems() const;

	// ========================= 구매 =========================

	// [Server] 아이템 구매 처리 — 검증(존재/중복/잔액) 후 GameInstance에 위임
	// TODO: 클라이언트에서 PlayerController Server RPC를 통해 호출해야 함
	bool BuyItem(APlayerController* Buyer, FGameplayTag ItemTag);

	// ========================= 데이터 =========================

	// 소모품(기능성) DataTable — 에디터(BP_ParcelGameState)에서 DT_ConsumableItems 지정
	UPROPERTY(EditAnywhere, Category = "Data")
	TObjectPtr<UDataTable> ConsumableDataTable;

	// 치장용 DataTable — 에디터(BP_ParcelGameState)에서 DT_CosmeticItems 지정
	UPROPERTY(EditAnywhere, Category = "Data")
	TObjectPtr<UDataTable> CosmeticDataTable;
};
