// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/TeamScoreComponent.h"
#include "Net/UnrealNetwork.h"
#include "ParcelLog.h"

// ========================= 초기화 =========================

UTeamScoreComponent::UTeamScoreComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	MinComboMultiplier = 1.0f;
	MaxComboMultiplier = 2.0f;
}

void UTeamScoreComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTeamScoreComponent, TeamScore);
	DOREPLIFETIME(UTeamScoreComponent, ComboCount);
	DOREPLIFETIME(UTeamScoreComponent, RemainingTime);
	DOREPLIFETIME(UTeamScoreComponent, CurrentGrade);
}

// ========================= 조회 =========================

int32 UTeamScoreComponent::GetTeamScore() const
{
	return TeamScore;
}

float UTeamScoreComponent::GetRemainingTime() const
{
	return RemainingTime;
}

FGameplayTag UTeamScoreComponent::GetGrade() const
{
	return CurrentGrade;
}

float UTeamScoreComponent::GetComboMultiplier()
{
	return FMath::Clamp(MinComboMultiplier + ComboCount * 0.1f, MinComboMultiplier, MaxComboMultiplier);
}

// ========================= 점수 =========================

void UTeamScoreComponent::AddTeamScore(int32 Amount)
{
	if (GetOwner()->HasAuthority())
	{
		TeamScore += Amount * GetComboMultiplier();
		TeamScore = FMath::Max(0, TeamScore);
		GAMERULE_LOG(Log, TEXT("[서버] TeamScore 변경 → %d, ComboCount → %d"), TeamScore, ComboCount);
		// [UI] 리슨 서버는 OnRep가 자동 호출되지 않으므로 수동 호출
		OnRep_TeamScore();
	}
}

// ========================= 콤보 =========================

void UTeamScoreComponent::OnDeliverySuccess()
{
	if (!GetOwner()->HasAuthority()) return;
	ComboCount++;
	OnRep_ComboCount();
}

void UTeamScoreComponent::OnDeliveryFail()
{
	if (!GetOwner()->HasAuthority()) return;
	ComboCount = 0;
	OnRep_ComboCount();
}

// ========================= 시간 =========================

void UTeamScoreComponent::InitRemainingTime(float InTimeLimit)
{
	if (GetOwner()->HasAuthority())
	{
		RemainingTime = InTimeLimit;
		// [UI] 초기화 시간 반영
		OnRep_RemainingTime();
	}
}

void UTeamScoreComponent::DecreaseRemainingTime(float Amount)
{
	if (GetOwner()->HasAuthority())
	{
		RemainingTime -= Amount;
		// [UI] 매 초 타이머UI 동기화
		OnRep_RemainingTime();
	}
}

// ========================= 등급 =========================

void UTeamScoreComponent::SetGrade(FGameplayTag InGrade)
{
	if (GetOwner()->HasAuthority())
		CurrentGrade = InGrade;
}

// ========================= 복제 콜백 =========================

void UTeamScoreComponent::OnRep_TeamScore()
{
	GAMERULE_LOG(Log, TEXT("[클라이언트] TeamScore 수신 → %d"), TeamScore);
	
	// [UI] 수신 시점에 위젯 델리게이트 브로드캐스트
	if (OnTeamScoreChanged.IsBound())
	{
		OnTeamScoreChanged.Broadcast(TeamScore);
	}
}

// [UI] 타이머 복제 알림
void UTeamScoreComponent::OnRep_RemainingTime()
{
	if (OnRemainingTimeChanged.IsBound())
	{
		OnRemainingTimeChanged.Broadcast(RemainingTime);
	}
}

// [UI] 콤보 카운트 복제 알림
void UTeamScoreComponent::OnRep_ComboCount()
{
	GAMERULE_LOG(Log, TEXT("[클라이언트] ComboCount 수신 → %d"), ComboCount);
	if (OnComboChanged.IsBound())
	{
		OnComboChanged.Broadcast(ComboCount);
	}
}

// [UI] 콤보 카운트 게터 함수
int32 UTeamScoreComponent::GetComboCount() const
{
	return ComboCount;
}