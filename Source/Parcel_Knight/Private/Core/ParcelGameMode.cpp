// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelGameMode.h"

#include "Character/ParcelCharacter.h"
#include "Core/DeliveryRuleComponent.h"
#include "Core/RespawnComponent.h"
#include "Core/ParcelGameState.h"
#include "Core/ParcelPlayerController.h"
#include "Core/ParcelPlayerState.h"
#include "Delivery/StageData.h"
#include "ParcelLog.h"
#include "Core/ParcelGameInstance.h"

AParcelGameMode::AParcelGameMode()
{
	GameStateClass      = AParcelGameState::StaticClass();
	PlayerStateClass    = AParcelPlayerState::StaticClass();
	PlayerControllerClass = AParcelPlayerController::StaticClass();
	DefaultPawnClass    = AParcelCharacter::StaticClass();
	DeliveryRuleComp = CreateDefaultSubobject<UDeliveryRuleComponent>("DeliveryRuleComponent");
	RespawnComp      = CreateDefaultSubobject<URespawnComponent>("RespawnComponent");
	
	bUseSeamlessTravel = true;
}

void AParcelGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}

void AParcelGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
}

// ========================= Match 상태 =========================

bool AParcelGameMode::ReadyToStartMatch_Implementation()
{
	// CurrentStageData가 설정될 때까지 Match 시작 대기 — Level BP BeginPlay 타이밍 보장
	return Super::ReadyToStartMatch_Implementation() && CurrentStageData != nullptr;
}

void AParcelGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	GAMERULE_LOG(Log, TEXT("Match Started — StartRound 호출"));
	StartRound(CurrentStageData);
}

void AParcelGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
	EndRound();
}

// ========================= 라운드 =========================

void AParcelGameMode::StartRound(UStageData* InStageData)
{
	if (!InStageData) return;
	DeliveryRuleComp->StartRound(InStageData);
}

void AParcelGameMode::EndRound()
{
	DeliveryRuleComp->EndRound();
}

// ========================= 배달 =========================

void AParcelGameMode::OnDeliveryCompleted(APlayerController* Deliverer, const FString& BoxName, int32 ScoreAmount)
{
	DeliveryRuleComp->OnDeliveryCompleted(Deliverer, BoxName, ScoreAmount);
}

void AParcelGameMode::OnDeliveryFailed(APlayerController* Deliverer, const FString& BoxName, int32 ScoreAmount)
{
	DeliveryRuleComp->OnDeliveryFailed(Deliverer, BoxName, ScoreAmount);
}

// ========================= 컴포넌트 접근 =========================

URespawnComponent* AParcelGameMode::GetRespawnComponent() const
{
	return RespawnComp;
}
