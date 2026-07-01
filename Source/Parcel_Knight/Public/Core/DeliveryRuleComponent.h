// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DeliveryRuleComponent.generated.h"

/**
 * 게임 룰 로직을 담당하는 컴포넌트
 * GameMode에 부착되며 점수 계산, 시간 관리, 배송 판정을 처리한다.
 *
 * 담당자: 한수현
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UDeliveryRuleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDeliveryRuleComponent();

	// [Server Only] 배송 판정은 서버에서만 — 결과는 GameState를 통해 클라이언트에 전파
	void EvaluateDelivery();

	// [Server Only] 택배 종류·콤보·감점을 반영한 최종 점수 반환
	int32 CalculateScore();
	
	// [Server Only] GameMode::StartRound()에서 호출 — 타이머 시작
	void StartRound();

	// [Server Only] GameMode::EndRound()에서 호출 — 타이머 정지
	void EndRound();

private:
	// 스테이지 제한시간 — DA_StageData에서 설정
	UPROPERTY()
	float TimeLimit;
	
	// 이 점수 달성 시 클리어 판정 — DA_StageData에서 설정
	UPROPERTY()
	int32 TargetScore;
	
	// "초" 당 감소되는 점수
	UPROPERTY()
	int32 DecreaseScore;
	
	//제한 시간 초과 시 감소되는 점수
	UPROPERTY()
	int32 TimeUpScore;
	
	// 1초 반복 타이머 핸들 — 시간 경과에 따른 점수 감소용
	FTimerHandle RoundTimerHandle;

	// 시간 만료 타이머 핸들 — TimeLimit 후 OnTimeUp 호출
	FTimerHandle TimeUpHandle;

	// [Server Only] 1초마다 호출 — 시간 경과 점수 감소 처리
	//HACK: 다른 코드로 변경 가능할 수 있음
	void OnEverySecond();
	
	/*
	 * [Server Only] 제한시간 만료 시 호출 — 점수 대폭 감소 및 라운드 종료
	*/
	void OnTimeUp();
};
