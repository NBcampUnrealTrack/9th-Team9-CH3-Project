#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameplayTagContainer.h"
#include "ParcelGameInstance.generated.h"

class UParcelSaveGame;

/**
 * 플레이어 영구 데이터를 런타임에 보관하는 GameInstance
 * 게임 시작 시 SaveGame에서 불러오고, 변경 시마다 AppData/Local/[프로젝트]/Saved/SaveGames/ 에 저장한다.
 *
 * 담당자: 한수현
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	// ========================= 저장 / 불러오기 =========================

	// [All] 현재 데이터를 SaveGames 폴더에 저장
	void SaveData();
	// [All] SaveGames 폴더에서 데이터를 불러와 캐시에 반영
	void LoadData();

	// ========================= 재화 =========================

	// [All] 현재 보유 재화 반환
	int32 GetMoney() const;
	// [All] 재화 추가 — 게임 종료 후 보상 지급 시 호출
	void AddMoney(int32 Amount);
	// [All] 재화 차감 — 잔액 부족 시 false 반환
	bool SpendMoney(int32 Amount);

	// ========================= 로드아웃 =========================

	// [All] 현재 로드아웃 반환 — UI 표시 및 스테이지 초기화용
	const TArray<FGameplayTag>& GetLoadout() const;
	// [All] 슬롯에 아이템 장착 — 실패 시 false 반환
	bool SetLoadoutSlot(int32 SlotIndex, FGameplayTag ItemTag);
	// [All] 슬롯 비우기
	void ClearLoadoutSlot(int32 SlotIndex);
	// [All] 현재 최대 슬롯 수 반환
	int32 GetMaxLoadoutSlots() const;
	// [Debug] 최대 슬롯 수 변경 — 밸런스 테스트용
	void SetMaxLoadoutSlots(int32 Count);
	// [Debug] 중복 장착 허용 여부 토글
	void ToggleDuplicateLoadout();
	// [Debug] 중복 허용 여부 반환
	bool IsAllowDuplicateLoadout() const;

private:
	static const FString SaveSlotName;

	// 런타임 캐시 — SaveGame 데이터를 메모리에 올려둔 복사본
	int32 Money = 0;
	TArray<FGameplayTag> OwnedConsumables;
	TArray<FGameplayTag> OwnedCosmetics;

	// 로드아웃 캐시
	TArray<FGameplayTag> EquippedLoadout;
	int32 MaxLoadoutSlots = 3;

	// 중복 장착 허용 플래그 — 저장하지 않고 디버그 전용으로 런타임에만 유지
	bool bAllowDuplicateLoadout = false;
};
