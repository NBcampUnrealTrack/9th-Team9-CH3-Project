#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "ParcelSaveGame.generated.h"

/**
 * 플레이어 영구 데이터를 저장하는 SaveGame 클래스
 * 재화, 보유 아이템(소모품/코스메틱)을 AppData/Local/[프로젝트]/Saved/SaveGames/ 에 직렬화한다.
 *
 * 담당자: 한수현
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 클리어한 최고 스테이지 번호 (0 = 아무것도 클리어 안 함)
	UPROPERTY(SaveGame)
	int32 MaxClearedStage = 0;

	// 보유 재화
	UPROPERTY(SaveGame)
	int32 Money = 0;

	// 보유 소모 아이템 (Item.Consumable.*)
	UPROPERTY(SaveGame)
	TArray<FGameplayTag> OwnedConsumables;

	// 보유 코스메틱 — Skin / Title / Effect 전부 포함 (Item.Cosmetic.*)
	UPROPERTY(SaveGame)
	TArray<FGameplayTag> OwnedCosmetics;

	// 스테이지에 들고갈 소모품 로드아웃 (게임 시작 전 선택)
	UPROPERTY(SaveGame)
	TArray<FGameplayTag> EquippedLoadout;

	// 장착 중인 코스메틱 — 태그가 유효하지 않으면 미장착 상태
	UPROPERTY(SaveGame)
	FGameplayTag EquippedSkin;

	UPROPERTY(SaveGame)
	FGameplayTag EquippedTitle;

	UPROPERTY(SaveGame)
	FGameplayTag EquippedEffect;
};
