// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/TeamScoreComponent.h"
#include "Net/UnrealNetwork.h"
#include "ParcelLog.h"


UTeamScoreComponent::UTeamScoreComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	CurrentGrade = EGrade::F;
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

void UTeamScoreComponent::OnRep_TeamScore()
{
	GAMERULE_LOG(Log, TEXT("[클라이언트] TeamScore 수신 → %d"), TeamScore);
	//TODO: UI작업 BroadCast(3단계)
}

void UTeamScoreComponent::InitRemainingTime(float InTimeLimit)
{
	if (GetOwner()->HasAuthority())
		RemainingTime = InTimeLimit;
}

void UTeamScoreComponent::AddTeamScore(int32 Amount)
{
	if (GetOwner()->HasAuthority())
	{
		TeamScore += Amount * GetComboMultiplier();
		GAMERULE_LOG(Log, TEXT("[서버] TeamScore 변경 → %d, ComboCount 변경 → %d"), TeamScore, ComboCount);
	}
}

void UTeamScoreComponent::DecreaseRemainingTime(float Amount)
{
	if (GetOwner()->HasAuthority())
	{
		RemainingTime -= Amount;
	}
}

float UTeamScoreComponent::GetComboMultiplier()
{
	return FMath::Clamp(MinComboMultiplier + ComboCount * 0.1f, MinComboMultiplier, MaxComboMultiplier);
}

void UTeamScoreComponent::OnDeliverySuccess()
{
	if (GetOwner()->HasAuthority())
	{
	ComboCount++;
	}
	
}

void UTeamScoreComponent::OnDeliveryFail()
{
	if (!GetOwner()->HasAuthority()) return;
	ComboCount = 0;
}


int32 UTeamScoreComponent::GetTeamScore() const
{
	return TeamScore;
}

float UTeamScoreComponent::GetRemainingTime() const
{
	return RemainingTime;
}

EGrade UTeamScoreComponent::GetGrade() const
{
	return CurrentGrade;
}

void UTeamScoreComponent::SetGrade(EGrade InGrade)
{
	if (GetOwner()->HasAuthority())
		CurrentGrade = InGrade;
}
