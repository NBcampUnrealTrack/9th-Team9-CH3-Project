// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/TeamScoreComponent.h"
#include "Net/UnrealNetwork.h"

UTeamScoreComponent::UTeamScoreComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UTeamScoreComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTeamScoreComponent, TeamScore);
	DOREPLIFETIME(UTeamScoreComponent, RemainingTime);
}

void UTeamScoreComponent::OnRep_TeamScore()
{
	//TODO: UI작업 BroadCast(3단계)
}

void UTeamScoreComponent::AddTeamScore(int32 Amount)
{
	if (GetOwner()->HasAuthority())
	{
		TeamScore += Amount;
	}
}

void UTeamScoreComponent::DecreaseRemainingTime(float Amount)
{
	if (GetOwner()->HasAuthority())
	{
		RemainingTime += Amount;
	}
}


int32 UTeamScoreComponent::GetTeamScore() const
{
	return TeamScore;
}

float UTeamScoreComponent::GetRemainingTime() const
{
	return RemainingTime;
}
