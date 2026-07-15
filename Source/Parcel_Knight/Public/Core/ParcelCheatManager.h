// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "ParcelCheatManager.generated.h"

/**
 * 개발용 콘솔 명령어 모음
 * UCheatManager를 상속받아 ~ 콘솔에서 입력 가능
 * AParcelGameMode::CheatClass에 등록되어 자동 생성됨
 *
 * 담당자: 한수현
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	// ─── 배달 ───
	// 배달 성공 시뮬레이션 — 콤보+1, 팀/개인 점수 +100
	UFUNCTION(Exec) void DebugDeliverySuccess();
	// 배달 실패 시뮬레이션 — 콤보 초기화
	UFUNCTION(Exec) void DebugDeliveryFail();
	// 팀 점수 직접 추가
	UFUNCTION(Exec) void DebugAddScore(int32 Amount);
	// 현재 팀 점수 및 콤보 배율 출력
	UFUNCTION(Exec) void DebugPrintScore();

	// ─── 재화 ───
	// 재화 직접 추가
	UFUNCTION(Exec) void DebugAddMoney(int32 Amount);
	// 현재 보유 재화 출력
	UFUNCTION(Exec) void DebugPrintMoney();

	// ─── 로드아웃 ───
	// 슬롯에 아이템 장착 (태그 문자열로 입력)
	UFUNCTION(Exec) void DebugSetLoadoutSlot(int32 SlotIndex, FString ItemTagStr);
	// 슬롯 비우기
	UFUNCTION(Exec) void DebugClearLoadoutSlot(int32 SlotIndex);
	// 현재 로드아웃 출력
	UFUNCTION(Exec) void DebugPrintLoadout();
	// 최대 슬롯 수 변경
	UFUNCTION(Exec) void DebugSetMaxSlots(int32 Count);
	// 중복 장착 허용 토글
	UFUNCTION(Exec) void DebugToggleDuplicateLoadout();

	// ─── 아이템 ───
	// 소모품 직접 지급 — 구매 없이 보유 목록에 추가
	UFUNCTION(Exec) void DebugAddConsumable(FString ItemTagStr);
	// 코스메틱 직접 지급 — 구매 없이 보유 목록에 추가
	UFUNCTION(Exec) void DebugAddCosmetic(FString ItemTagStr);
	// 보유 중인 소모품/코스메틱 전체 출력
	UFUNCTION(Exec) void DebugPrintOwnedItems();
	// 상점 구매 흐름 시뮬레이션 — BuyItem 직접 호출
	UFUNCTION(Exec) void DebugBuyItem(FString ItemTagStr);
	// 현재 스테이지 인벤토리(InventoryComponent) 출력
	UFUNCTION(Exec) void DebugPrintInventory();
};
