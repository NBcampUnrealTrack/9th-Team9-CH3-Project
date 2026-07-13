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

	// ========================= 사용 =========================

	// [Server] 아이템 사용 — 영구 아이템이면 제거하지 않음, 없으면 false 반환
	bool UseItem(FGameplayTag ItemTag);

	// ========================= 이벤트 =========================

	// 인벤토리 변경 시 브로드캐스트 — UI 갱신용
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	// ========================= 데이터 =========================

	// 소모품 DataTable — 에디터(BP)에서 DT_ConsumableItems 지정
	UPROPERTY(EditAnywhere, Category = "Data")
	TObjectPtr<UDataTable> ConsumableDataTable;

private:
	// 서버에서 변경 후 클라이언트에 복제 — OnRep에서 UI 갱신 델리게이트 호출
	UFUNCTION()
	void OnRep_Items();

	UPROPERTY(ReplicatedUsing = OnRep_Items)
	TArray<FGameplayTag> Items;
};
