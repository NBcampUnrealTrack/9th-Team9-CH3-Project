// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelGameMode.h"

#include "Character/ParcelCharacter.h"
#include "Core/DeliveryRuleComponent.h"
#include "Core/ParcelGameState.h"
#include "Core/TeamScoreComponent.h"
#include "Core/ParcelPlayerController.h"
#include "Core/ParcelPlayerState.h"
#include "Delivery/StageData.h"
#include "ParcelLog.h"

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
	GAMERULE_LOG(Log, TEXT("Match Started — StartRound 호출"));
	//TODO:StartRound(스테이지데이터) 예정
}

void AParcelGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
	EndRound();
}

void AParcelGameMode::StartRound(UStageData* InStageData)
{
	if (!InStageData) return;
	DeliveryRuleComp->StartRound(InStageData->TimeLimit, InStageData->TargetScore);
}

void AParcelGameMode::EndRound()
{
	DeliveryRuleComp->EndRound();
}

void AParcelGameMode::OnDeliveryCompleted(APlayerController* Deliverer, int32 ScoreAmount)
{
	DeliveryRuleComp->OnDeliveryCompleted(Deliverer, ScoreAmount);
}

void AParcelGameMode::OnDeliveryFailed(APlayerController* Deliverer, int32 ScoreAmount)
{
	DeliveryRuleComp->OnDeliveryFailed(Deliverer, ScoreAmount);
}

// ========================= 콘솔 명령어 =========================

void AParcelGameMode::DebugDeliverySuccess()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		OnDeliveryCompleted(PC, 100);
	GAMERULE_LOG(Log, TEXT("[콘솔] 배달 성공 시뮬레이션"));
}

void AParcelGameMode::DebugDeliveryFail()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		OnDeliveryFailed(PC);
	GAMERULE_LOG(Log, TEXT("[콘솔] 배달 실패 시뮬레이션"));
}

void AParcelGameMode::DebugAddScore(int32 Amount)
{
	AParcelGameState* GS = GetGameState<AParcelGameState>();
	if (!GS) return;

	GS->GetTeamScoreComponent()->AddTeamScore(Amount);
	GAMERULE_LOG(Log, TEXT("[콘솔] 점수 추가: %d"), Amount);
}

void AParcelGameMode::DebugPrintScore()
{
	AParcelGameState* GS = GetGameState<AParcelGameState>();
	if (!GS) return;

	UTeamScoreComponent* TSC = GS->GetTeamScoreComponent();
	FString Msg = FString::Printf(TEXT("TeamScore: %d | 콤보배율: %.1fx"),
		TSC->GetTeamScore(), TSC->GetComboMultiplier());

	GAMERULE_LOG(Log, TEXT("[콘솔] %s"), *Msg);
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Msg);
}
