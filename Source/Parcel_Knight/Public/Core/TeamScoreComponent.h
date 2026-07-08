// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TeamScoreComponent.generated.h"

//TODO: 나중에 결과에 따라 애니메이션/UI 출력이 바뀌거나 한다면 확장성을 고려했을 때 Tag로 바꾸는 것이 좋아보임
UENUM(BlueprintType)
enum class EGrade : uint8
{
	A,
	B,
	C,
	F
};

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
	// [All] 최종 등급 반환
	EGrade GetGrade() const;
	// [Server Only] 팀 점수 증감
	void AddTeamScore(int32 Amount);
	// [Server Only] 남은 시간 감소 — OnEverySecond에서 호출
	void DecreaseRemainingTime(float Amount);
	// [Server Only] 남은 시간 초기화 — StartRound에서 호출
	void InitRemainingTime(float InTimeLimit);
	// [Server Only] 등급 설정 — OnTimeUp에서 호출
	void SetGrade(EGrade InGrade);
	// [Server Only] 배달 성공 — 콤보 증가 
	void OnDeliverySuccess();
	// [Server Only] 배달 실패 — 콤보 0 변경
	void OnDeliveryFail();
	// [Server Only] 콤보 배율
	float GetComboMultiplier();
	

private:
	// 모든 클라이언트에 복제 — UI 점수판 갱신용
	UPROPERTY(ReplicatedUsing=OnRep_TeamScore)
	int32 TeamScore;
	
	//점수 배율을 적용시키는 콤보 카운트
	UPROPERTY(Replicated)
	int32 ComboCount;

	// 남은 시간 복제 — 서버 타이머 기준, 클라이언트는 읽기만
	UPROPERTY(Replicated)
	float RemainingTime;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"))
	float MinComboMultiplier;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"))
	float MaxComboMultiplier;
	
	// 최종 등급 복제 — 결과 화면 표시용 (3단계)
	UPROPERTY(Replicated)
	EGrade CurrentGrade;
	
	
};
