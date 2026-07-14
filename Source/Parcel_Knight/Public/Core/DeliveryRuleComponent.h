// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DeliveryRuleComponent.generated.h"

class APlayerController;
class AParcelPlayerState;
class UStageData;

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
	
	// [Server Only] GameMode::StartRound()에서 호출 — 타이머 시작
	void StartRound(UStageData* InStageData);

	// [Server Only] GameMode::EndRound()에서 호출 — 타이머 정지
	void EndRound();

	// [Server Only] 배달 성공 시 호출 — 팀/개인 점수 및 콤보 처리
	void OnDeliveryCompleted(APlayerController* Deliverer, int32 ScoreAmount);

	// [Server Only] 배달 실패 시 호출 — 콤보 리셋 및 실패 횟수 처리
	void OnDeliveryFailed(APlayerController* Deliverer);

private:
	// 현재 라운드 스테이지 데이터 — StartRound에서 설정, OnTimeUp에서 등급/보상 산정에 사용
	UPROPERTY()
	TObjectPtr<UStageData> CurrentStageData;

	// 초당 감소 점수 — 빠른 배달을 유도하는 압박 장치
	UPROPERTY(EditAnywhere, Category = "Stage")
	int32 DecreaseScore;

	// 1초 반복 타이머 핸들 — 시간 경과에 따른 점수 감소용
	FTimerHandle RoundTimerHandle;

	// 시간 만료 타이머 핸들 — TimeLimit 후 OnTimeUp 호출
	FTimerHandle TimeUpHandle;

	// [Server Only] 1초마다 호출 — 시간 경과 점수 감소 처리
	//HACK: 다른 코드로 변경 가능할 수 있음
	void OnEverySecond();
	
	// [Server Only] 제한시간 만료 시 호출 — 점수 감소 및 라운드 종료
	void OnTimeUp();
};
