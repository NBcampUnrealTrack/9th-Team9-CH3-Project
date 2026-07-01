// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerStatComponent.generated.h"

/**
 * 개인 점수, 콤보, 성공·실패 횟수를 관리하는 컴포넌트
 * PlayerState에 부착된다.
 *
 * 담당자: 한수현
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UPlayerStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerStatComponent();

	
	/*
	 * Replicated로 클라이언트에 보내지는 변수들을 복사하는 함수
	 * 엔진이 호출하는 함수라 관례상 public에 둔다
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ─────────────────────────────────────
	// 조회
	// ─────────────────────────────────────

	// [All] 개인 누적 점수 반환
	int32 GetPersonalScore() const;

	// [All] 현재 콤보 횟수 반환
	int32 GetComboCount() const;

	// [All] 배송 성공 횟수 반환
	int32 GetSuccessCount() const;

	// ─────────────────────────────────────
	// 점수 및 콤보 처리
	// ─────────────────────────────────────

	/**
	 * [Server Only] 배송 성공·감점 결과를 개인 점수에 반영
	 * @param Amount 양수면 획득, 음수면 감점
	 */
	void AddScore(int32 Amount);

	// [Server Only] 배송 성공 시 호출 — 콤보 증가 및 성공 횟수 누적
	void OnDeliverySuccess();

	// [Server Only] 배송 실패·파손 시 호출 — 콤보 리셋
	void OnDeliveryFail();

private:
	UPROPERTY(Replicated)
	int32 PersonalScore;  // 개인 누적 점수

	UPROPERTY(Replicated)
	int32 SuccessCount;   // 연속 성공 콤보 계산 기준

	UPROPERTY(Replicated)
	int32 ComboCount;     // 콤보 배율 적용에 사용
};
