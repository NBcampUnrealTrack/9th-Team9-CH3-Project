#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameplayTagContainer.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ParcelGameInstance.generated.h"

class UParcelSaveGame;
class UDataTable;

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
	virtual void Shutdown() override;
	virtual void ReturnToMainMenu() override;

	// ========================= 저장 / 불러오기 =========================

	// [All] 현재 데이터를 SaveGames 폴더에 저장
	bool SaveData();
	// [All] SaveGames 폴더에서 데이터를 불러와 캐시에 반영
	void LoadData();

	// ========================= 재화 =========================

	// [All] 현재 보유 재화 반환
	int32 GetMoney() const;
	// [All] 재화 추가 — 게임 종료 후 보상 지급 시 호출
	void AddMoney(int32 Amount);
	// [All] 재화 차감 — 잔액 부족 시 false 반환
	bool SpendMoney(int32 Amount);

	// ========================= 보유 아이템 =========================

	// [All] 소모품 보유 여부 — 중복 구매 방지 및 로드아웃 장착 검증용
	bool HasOwnedConsumable(FGameplayTag ItemTag) const;
	// [All] 코스메틱 보유 여부 — 중복 구매 방지용
	bool HasOwnedCosmetic(FGameplayTag ItemTag) const;
	// [All] 소모품 소유 목록에 추가 — 구매 완료 후 호출
	void AddOwnedConsumable(FGameplayTag ItemTag);
	// [All] 코스메틱 소유 목록에 추가 — 구매 완료 후 호출
	void AddOwnedCosmetic(FGameplayTag ItemTag);
	// [All] 보유 소모품 전체 목록 반환 — 디버그/UI용
	const TArray<FGameplayTag>& GetOwnedConsumables() const;
	// [All] 보유 코스메틱 전체 목록 반환 — 디버그/UI용
	const TArray<FGameplayTag>& GetOwnedCosmetics() const;

	/**
	 * Frontend local-profile purchase entry point.
	 * The supplied table and ItemTag are validated before money or ownership is changed,
	 * and the completed transaction is written to the existing ParcelSaveGame in one save.
	 */
	bool TryPurchaseConsumable(UDataTable* ItemTable, FGameplayTag ItemTag, FText& OutStatusMessage);

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

	/** Toggle a locally owned consumable in the persistent frontend loadout. */
	bool ToggleLoadoutItem(FGameplayTag ItemTag, int32 RequestedMaxItems, FText& OutStatusMessage);

	/** Remove stale, duplicate, unowned, or no-longer-table-backed loadout entries. */
	bool ValidateSavedLoadout(UDataTable* ItemTable, int32 RequestedMaxItems);

	// ========================= 스테이지 진행도 =========================

	// [All] 클리어한 최고 스테이지 번호 반환 (0 = 아무것도 클리어 안 함)
	UFUNCTION(BlueprintPure)
	int32 GetMaxClearedStage() const;

	// [All] 스테이지 클리어 시 호출 — 갱신이 일어나면 즉시 저장
	UFUNCTION(BlueprintCallable)
	void UpdateMaxClearedStage(int32 ClearedStageIndex);

	// [Blueprint] N번 스테이지 진입 가능 여부 반환 (1번은 항상 true)
	UFUNCTION(BlueprintPure)
	bool IsStageUnlocked(int32 StageIndex) const;

	// ========================= 스테이지 맵 =========================

	// [Blueprint] 세션 생성 전 이동할 스테이지 맵 경로 설정 — UI에서 스테이지 선택 시 호출
	UFUNCTION(BlueprintCallable)
	void SetPendingMapPath(const FString& MapPath);

	// [SessionSubsystem] 세션 생성 완료 시 이동할 맵 경로 반환
	UFUNCTION(BlueprintPure)
	FString GetPendingMapPath() const;

	// ========================= 커스터마이징 =========================

	// [All] 스킨 장착 — 미보유 시 무시, 성공하면 저장
	UFUNCTION(BlueprintCallable, Category = "Parcel|Customization")
	void EquipSkin   (FGameplayTag SkinTag);
	// [All] 칭호 장착
	UFUNCTION(BlueprintCallable, Category = "Parcel|Customization")
	void EquipTitle  (FGameplayTag TitleTag);
	// [All] 이펙트 장착
	UFUNCTION(BlueprintCallable, Category = "Parcel|Customization")
	void EquipEffect (FGameplayTag EffectTag);

	// [All] 현재 장착된 스킨 태그 반환 — 미장착이면 Invalid 태그
	UFUNCTION(BlueprintPure, Category = "Parcel|Customization")
	FGameplayTag GetEquippedSkin()   const;
	// [All] 현재 장착된 칭호 태그 반환
	UFUNCTION(BlueprintPure, Category = "Parcel|Customization")
	FGameplayTag GetEquippedTitle()  const;
	// [All] 현재 장착된 이펙트 태그 반환
	UFUNCTION(BlueprintPure, Category = "Parcel|Customization")
	FGameplayTag GetEquippedEffect() const;

private:
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ApplyLocalUserSettings(const UObject* WorldContextObject);

	void HandleSessionInviteAccepted(
		bool bWasSuccessful,
		int32 ControllerId,
		FUniqueNetIdPtr UserId,
		const FOnlineSessionSearchResult& InviteResult);

	static const FString SaveSlotName;

	FDelegateHandle SessionInviteAcceptedHandle;
	IOnlineSessionPtr SessionInviteSessionInterface;
	FDelegateHandle PostLoadMapWithWorldHandle;

	// 런타임 캐시 — SaveGame 데이터를 메모리에 올려둔 복사본
	int32 MaxClearedStage = 0;
	int32 Money = 0;
	TArray<FGameplayTag> OwnedConsumables;
	TArray<FGameplayTag> OwnedCosmetics;

	// 로드아웃 캐시
	TArray<FGameplayTag> EquippedLoadout;
	int32 MaxLoadoutSlots = 3;

	// 중복 장착 허용 플래그 — 저장하지 않고 디버그 전용으로 런타임에만 유지
	bool bAllowDuplicateLoadout = false;

	// 세션 생성은 항상 Lobby에서 시작하고, 실제 Stage 이동은 SessionSubsystem::StartGame이 담당합니다.
	FString PendingMapPath = TEXT("/Game/Maps/LV_DF_Lobby_Stage00");

	// 장착 중인 코스메틱 캐시 — 태그가 유효하지 않으면 미장착
	FGameplayTag EquippedSkin;
	FGameplayTag EquippedTitle;
	FGameplayTag EquippedEffect;
};
