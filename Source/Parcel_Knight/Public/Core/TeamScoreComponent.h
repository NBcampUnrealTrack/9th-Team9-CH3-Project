// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TeamScoreComponent.generated.h"

/**
 * 팀 점수와 남은 시간을 관리하는 컴포넌트
 * GameState에 부착되며 모든 클라이언트에 점수와 시간을 복제한다.
 *
 * 담당자: 한수현
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UTeamScoreComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTeamScoreComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// [Multicast] TeamScore 변경 시 자동 호출 — 클라이언트 UI 갱신 트리거
	UFUNCTION()
	void OnRep_TeamScore();
	// [All] 현재 팀 점수 반환
	int32 GetTeamScore() const;
	// [All] 남은 시간 반환
	float GetRemainingTime() const;
	// [Server Only] 팀 점수 증감
	void AddTeamScore(int32 Amount);
	// [Server Only] 남은 시간 감소 — OnEverySecond에서 호출
	void DecreaseRemainingTime(float Amount);

private:
	// 모든 클라이언트에 복제 — UI 점수판 갱신용
	UPROPERTY(ReplicatedUsing=OnRep_TeamScore)
	int32 TeamScore;

	// 남은 시간 복제 — 서버 타이머 기준, 클라이언트는 읽기만
	UPROPERTY(Replicated)
	float RemainingTime;
};
