// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelGameState.h"

#include "Core/TeamScoreComponent.h"

AParcelGameState::AParcelGameState()
{
	TeamScoreComp = CreateDefaultSubobject<UTeamScoreComponent>("TeamScoreComponent");
}

UTeamScoreComponent* AParcelGameState::GetTeamScoreComponent() const
{
	return TeamScoreComp;
}
