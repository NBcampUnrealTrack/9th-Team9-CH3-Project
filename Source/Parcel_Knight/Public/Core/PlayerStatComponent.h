// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerStatComponent.generated.h"

// [UI]
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPersonalScoreChangedSignature, int32, NewPersonalScore);

/**
 * 개인 점수, 성공·실패 횟수를 관리하는 컴포넌트
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

	// [All] 배송 성공 횟수 반환
	int32 GetSuccessCount() const;

	// [All] 배송 실패 횟수 반환
	int32 GetFailCount() const;

	// ─────────────────────────────────────
	// 점수 처리
	// ─────────────────────────────────────

	/**
	 * [Server Only] 배송 성공·감점 결과를 개인 점수에 반영
	 * @param Amount 양수면 획득, 음수면 감점
	 */
	void AddScore(int32 Amount);

	// [Server Only] 배송 성공 시 호출 — 성공 횟수 누적
	void OnDeliverySuccess();

	// [Server Only] 배송 실패·파손 시 호출 — 실패 횟수 누적
	void OnDeliveryFail();

	// [UI] HUD 위젯 바인딩 브로드캐스트 변수
	UPROPERTY(BlueprintAssignable, Category = "ParcelUI|Events")
	FOnPersonalScoreChangedSignature OnPersonalScoreChanged;

	// [UI] 개인 점수 복제 수신 콜백 함수
	UFUNCTION()
	void OnRep_PersonalScore();
	
private:
	UPROPERTY(ReplicatedUsing=OnRep_PersonalScore)
	int32 PersonalScore;  // 개인 누적 점수

	UPROPERTY(Replicated)
	int32 SuccessCount;   // 배달 성공 횟수

	UPROPERTY(Replicated)
	int32 FailCount;      // 배달 실패 횟수
};
