#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "InventoryComponent.generated.h"

class UParcelGameInstance;
class UDataTable;

// 인벤토리 변경 시 UI 갱신용 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

/**
 * 스테이지 내 소모품 인벤토리를 관리하는 컴포넌트
 * PlayerState에 부착되며, 서버에서 초기화 후 클라이언트에 복제된다.
 * 영구 아이템(bIsPermanent)은 사용해도 제거되지 않는다.
 *
 * 담당자: 한수현
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ========================= 초기화 =========================

	// [Server] 스테이지 시작 시 GameInstance의 EquippedLoadout을 복사해 인벤토리 초기화
	// TODO: 원격 클라이언트는 PlayerController에서 Server RPC로 로드아웃을 전달받아 별도 초기화 필요
	void InitFromGameInstance(UParcelGameInstance* GI);

	// ========================= 조회 =========================

	// [All] 아이템 보유 여부 — 캐릭터가 사용 전 검증용
	bool HasItem(FGameplayTag ItemTag) const;

	// [All] 현재 보유 아이템 목록 반환 — UI 표시용
	const TArray<FGameplayTag>& GetItems() const;

	/**
	 * Server-only bridge from a client's persistent frontend selection to the replicated stage inventory.
	 * Only unique entries present in ConsumableDataTable are accepted.
	 */
	bool SetValidatedLoadout(const TArray<FGameplayTag>& RequestedItems, int32 MaxItems = 3);

	// [Server] 쿨타임·보유 여부를 모두 검사 — 사용 가능하면 true
	bool CanUseItem(FGameplayTag ItemTag) const;

	// ========================= 추가·사용 =========================

	// [Server] 아이템 추가 — 치트·보상 지급용
	void AddItem(FGameplayTag ItemTag);

	// [Server] 스폰 시 패시브 소모품 효과 일괄 적용 — PossessedBy에서 호출
	void ApplyPassiveEffects(APawn* Pawn);

	// [Server] 아이템 사용 — 쿨타임·효과 적용, 비영구 아이템은 제거. 실패 시 false
	bool UseItem(FGameplayTag ItemTag);

	// [Server via Client RPC] UI에서 아이템 사용 요청 — UseItem을 서버에서 실행
	UFUNCTION(Server, Reliable)
	void Server_UseItem(FGameplayTag ItemTag);

	// [Multicast, Unreliable] 아이템 사용 성공 시 모든 클라이언트에서 FX/Sound 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayItemFX(FGameplayTag ItemTag);

	// ========================= 이벤트 =========================

	// 인벤토리 변경 시 브로드캐스트 — UI 갱신용
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	// ========================= 데이터 =========================

	// 소모품 DataTable — 에디터(BP)에서 DT_ConsumableItems 지정
	UPROPERTY(EditAnywhere, Category = "Data")
	TObjectPtr<UDataTable> ConsumableDataTable;

private:
	UFUNCTION()
	void OnRep_Items();

	UPROPERTY(ReplicatedUsing = OnRep_Items)
	TArray<FGameplayTag> Items;

	// 아이템별 마지막 사용 시각 — 서버 전용, 쿨타임 계산용
	TMap<FGameplayTag, float> ItemLastUsedTime;

	// DataTable에서 ItemTag에 해당하는 행 검색
	struct FItemData* FindItemData(FGameplayTag ItemTag) const;

	// 효과 배열을 순회하며 캐릭터에 적용
	void ApplyEffects(const struct FItemData* Data, APawn* Pawn);
};
