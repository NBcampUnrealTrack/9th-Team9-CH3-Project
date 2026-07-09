// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ParcelPlayerState.generated.h"

class UPlayerStatComponent;
class UInventoryComponent;

/**
 * 개인 점수, 성공·실패 횟수를 관리하는 PlayerState
 * 실제 로직은 UPlayerStatComponent가 담당한다.
 *
 * 담당자: 한수현
 */
UCLASS()
class PARCEL_KNIGHT_API AParcelPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	
	AParcelPlayerState();

	// [All] 개인 누적 점수 반환
	int32 GetPersonalScore() const;
	// [All] 배달 성공 횟수 반환
	int32 GetSuccessCount() const;
	// [All] 배달 실패 횟수 반환
	int32 GetFailCount() const;

	// [Server Only] 배달 성공·감점 결과를 개인 점수에 반영
	void AddScore(int32 Amount);
	// [Server Only] 배달 성공 시 호출 — 성공 횟수 누적
	void OnDeliverySuccess();
	// [Server Only] 배달 실패·파손 시 호출 — 실패 횟수 누적
	void OnDeliveryFail();

	// [All] 인벤토리 컴포넌트 반환 — 캐릭터의 아이템 사용 시 참조
	UInventoryComponent* GetInventoryComponent() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UPlayerStatComponent> PlayerStatComp;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> InventoryComp;
};
