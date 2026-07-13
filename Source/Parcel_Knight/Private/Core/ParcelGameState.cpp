// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelGameState.h"

#include "Core/TeamScoreComponent.h"
#include "Core/ShopComponent.h"

AParcelGameState::AParcelGameState()
{
	TeamScoreComp = CreateDefaultSubobject<UTeamScoreComponent>("TeamScoreComponent");
	ShopComp      = CreateDefaultSubobject<UShopComponent>("ShopComponent");
	
	// [UI] 클라이언트에 의해 복제 허용
	TeamScoreComp->SetIsReplicated(true);
}

UTeamScoreComponent* AParcelGameState::GetTeamScoreComponent() const
{
	return TeamScoreComp;
}

UShopComponent* AParcelGameState::GetShopComponent() const
{
	return ShopComp;
}
