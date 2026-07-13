// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ParcelGameMode.generated.h"

class AParcelPlayerController;
class AParcelPlayerState;
class AParcelCharacter;
class UDeliveryRuleComponent;
class AParcelGameState;
class UStageData;

/**
 * 게임 룰을 관리하는 GameMode
 * 실제 로직은 UDeliveryRuleComponent가 담당한다.
 *
 * 담당자: 한수현
 */
UCLASS()
class PARCEL_KNIGHT_API AParcelGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AParcelGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	void StartRound(UStageData* InStageData);
	void EndRound();

	// 배달 성공/실패 진입점 — 다른 팀원 코드에서 이 함수만 호출
	void OnDeliveryCompleted(APlayerController* Deliverer, int32 ScoreAmount);
	void OnDeliveryFailed(APlayerController* Deliverer);

protected:
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UDeliveryRuleComponent> DeliveryRuleComp;
	
	
	// ========================= 콘솔 명령어 =========================
	// NOTE: ~(콘솔)에서 입력 시 실제 해당 코드가 실행됨
	// 실제 로직은 각 담당 클래스로 위임한다
	//   배달 관련 → DeliveryRuleComponent
	//   재화 관련 → GameInstance

	public:
	// [배달] 배달 성공 시뮬레이션 — 콤보+1, 팀/개인 점수 +100
	UFUNCTION(Exec) void DebugDeliverySuccess();
	// [배달] 배달 실패 시뮬레이션 — 콤보 초기화
	UFUNCTION(Exec) void DebugDeliveryFail();
	// [배달] 팀 점수 직접 추가
	UFUNCTION(Exec) void DebugAddScore(int32 Amount);
	// [배달] 현재 팀 점수 및 콤보 배율 출력
	UFUNCTION(Exec) void DebugPrintScore();
	
	//------------ 세이브 관련 콘솔코드----------
	//   재화 관련 → GameInstance
	//   로드아웃 관련 → GameInstance

	// [재화] 재화 직접 추가
	UFUNCTION(Exec) void DebugAddMoney(int32 Amount);
	// [재화] 현재 보유 재화 출력
	UFUNCTION(Exec) void DebugPrintMoney();
	// [로드아웃] 슬롯에 아이템 장착 (태그 문자열로 입력)
	UFUNCTION(Exec) void DebugSetLoadoutSlot(int32 SlotIndex, FString ItemTagStr);
	// [로드아웃] 슬롯 비우기
	UFUNCTION(Exec) void DebugClearLoadoutSlot(int32 SlotIndex);
	// [로드아웃] 현재 로드아웃 출력
	UFUNCTION(Exec) void DebugPrintLoadout();
	// [로드아웃] 최대 슬롯 수 변경
	UFUNCTION(Exec) void DebugSetMaxSlots(int32 Count);
	// [로드아웃] 중복 장착 허용 토글
	UFUNCTION(Exec) void DebugToggleDuplicateLoadout();

	//------------ 아이템 관련 콘솔코드 ----------
	//   아이템 관련 → GameInstance / ShopComponent / InventoryComponent

	// [아이템] 소모품 직접 지급 — 구매 없이 보유 목록에 추가
	UFUNCTION(Exec) void DebugAddConsumable(FString ItemTagStr);
	// [아이템] 코스메틱 직접 지급 — 구매 없이 보유 목록에 추가
	UFUNCTION(Exec) void DebugAddCosmetic(FString ItemTagStr);
	// [아이템] 보유 중인 소모품/코스메틱 전체 출력
	UFUNCTION(Exec) void DebugPrintOwnedItems();
	// [아이템] 상점 구매 흐름 시뮬레이션 — BuyItem 직접 호출
	UFUNCTION(Exec) void DebugBuyItem(FString ItemTagStr);
	// [아이템] 현재 스테이지 인벤토리(InventoryComponent) 출력
	UFUNCTION(Exec) void DebugPrintInventory();
};



