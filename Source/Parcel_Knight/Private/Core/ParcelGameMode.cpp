// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelGameMode.h"

#include "Character/ParcelCharacter.h"
#include "Core/DeliveryRuleComponent.h"
#include "Core/ParcelGameState.h"
#include "Core/ParcelPlayerController.h"
#include "Core/ParcelPlayerState.h"

AParcelGameMode::AParcelGameMode()
{
	GameStateClass = AParcelGameState::StaticClass();
	PlayerStateClass = AParcelPlayerState::StaticClass();
	PlayerControllerClass = AParcelPlayerController::StaticClass();
	DefaultPawnClass = AParcelCharacter::StaticClass();

	DeliveryRuleComp = CreateDefaultSubobject<UDeliveryRuleComponent>("DeliveryRuleComponent");

}

void AParcelGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}

void AParcelGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
}

void AParcelGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	StartRound();
}

void AParcelGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
	EndRound();
}

void AParcelGameMode::StartRound()
{
	DeliveryRuleComp->StartRound();
}

void AParcelGameMode::EndRound()
{
	DeliveryRuleComp->EndRound();
}
