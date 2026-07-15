// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
#include "ParcelPlayerState.generated.h"

class UPlayerStatComponent;
class UInventoryComponent;
class UCustomizationComponent;

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
	// [All] 사망 횟수 반환
	int32 GetDeathCount() const;

	// [Server Only] 배달 성공·감점 결과를 개인 점수에 반영
	void AddScore(int32 Amount);
	// [Server Only] 배달 성공 시 호출 — 성공 횟수 누적
	void OnDeliverySuccess();
	// [Server Only] 배달 실패·파손 시 호출 — 실패 횟수 누적
	void OnDeliveryFail();

	// [Server Only] 사망 시 호출 — DeathCount 증가 + 리스폰 타이머 시작
	// 캐릭터의 HealthComponent OnDeathDelegate에서 호출할 것
	void HandleDeath();

	// [Client] 라운드 종료 보상 수령 — GameMode에서 각 플레이어에게 호출
	UFUNCTION(Client, Reliable)
	void Client_GrantReward(int32 RewardAmount);

	// [All] 인벤토리 컴포넌트 반환 — 캐릭터의 아이템 사용 시 참조
	UInventoryComponent* GetInventoryComponent() const;

	// [All] 커스터마이징 컴포넌트 반환 — 캐릭터 스폰 시 장착 정보 조회용
	UCustomizationComponent* GetCustomizationComponent() const;

	// NOTE: 커스터마이징 장착 패턴 두 가지
	/*
	//   [확정 장착] PlayerState->EquipSkin(Tag)
	//     GameInstance 저장 + 컴포넌트 반영을 동시에 처리.
	//     게임을 껐다 켜도 유지됨.
	//
	//   [미리보기]  PlayerState->GetCustomizationComponent()->EquipSkin(Tag)
	//     컴포넌트만 갱신, GameInstance 저장 없음.
	//     게임을 껐다 켜면 원래 장착으로 돌아옴.
	//     취소 시 GetEquipped*()로 원래 태그를 읽어서 되돌릴 것.
	*/
	// [All] 스킨  장착 — GameInstance(저장)·CustomizationComponent(반영) 두 레이어를 묶는 편의 함수들
	void EquipSkin   (FGameplayTag SkinTag);
	// [All] 칭호 장착
	void EquipTitle  (FGameplayTag TitleTag);
	// [All] 이펙트  장착
	void EquipEffect (FGameplayTag EffectTag);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UPlayerStatComponent> PlayerStatComp;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> InventoryComp;

	UPROPERTY()
	TObjectPtr<UCustomizationComponent> CustomizationComp;
};
