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
#include "Delivery/StageData.h"
#include "Delivery/DeliverySubsystem.h"
#include "Delivery/DeliveryBoxSpawner.h"
#include "EngineUtils.h"

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
	
	if (DefaultStageData)
	{
		StartRound(DefaultStageData);
		GAMERULE_LOG(Log, TEXT("성공적으로 StageData를 로드하여 모든 시스템을 활성화했습니다."));
	}
	else
	{
		GAMERULE_LOG(Error, TEXT("DefaultStageData가 지정되지 않았습니다. BP_ParcelGameMode에서 할당해주세요."));
	}
}

void AParcelGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
	EndRound();
}

void AParcelGameMode::StartRound(UStageData* InStageData)
{
	if (!InStageData) return;
	
	// 타이머 시작
	if (DeliveryRuleComp)
	{
		DeliveryRuleComp->StartRound(InStageData->TimeLimit, InStageData->TargetScore);
	}
	
	// 서브시스템 초기화 + 월드 스폰 포인트 추적
	if (GetWorld())
	{
		UDeliverySubsystem* DeliverySubsystem = GetWorld()->GetSubsystem<UDeliverySubsystem>();
		if (DeliverySubsystem)
		{
			// 에디터에서 배치해놓은 박스 스포너 액터 추적
			DeliverySubsystem->InitializeStage(InStageData);
			
			TArray<ADeliveryBoxSpawner*> SpawnPoints;

			if (GetWorld())
			{
				for (ADeliveryBoxSpawner* Spawner : TActorRange<ADeliveryBoxSpawner>(GetWorld()))
				{
					if (Spawner)
					{
						SpawnPoints.Add(Spawner);
					}
				}
			}

			if (SpawnPoints.IsEmpty())
			{
				GAMERULE_LOG(Error, TEXT("[GameMode] 레벨에 배정된 ADeliveryBoxSpawner가 하나도 없습니다. 임시 좌표에 스폰합니다."));
			}

			int32 SpawnPointIndex = 0;
			int32 TotalSpawnedCount = 0;
			
			// StageData의 목록을 받아와 상자 스폰 + 위치 매핑
			for (const FStageBoxSpawnInfo& SpawnInfo : InStageData->BoxDataList)
			{
				if (!SpawnInfo.BoxTypeTag.IsValid()) continue;

				for (int32 i = 0; i < SpawnInfo.SpawnCount; ++i)
				{
					FVector SpawnLocation;
					FRotator SpawnRotation = FRotator::ZeroRotator;

					// 배치된 스폰 포인트가 있다면 그 위치를 쓰고, 부족하면 기본 공중 좌표 제공(예외 처리)
					if (SpawnPoints.IsValidIndex(SpawnPointIndex))
					{
						SpawnLocation = SpawnPoints[SpawnPointIndex]->GetActorLocation();
						SpawnRotation = SpawnPoints[SpawnPointIndex]->GetActorRotation();
						// 다음 상자는 다음 스폰 포인트로
						SpawnPointIndex++;
					}
					else
					{
						// 스폰 포인트 개수보다 스폰할 상자가 더 많을 때의 예외 안전장치
						SpawnLocation = FVector(0.f, 0.f, 400.f) + FVector(i * 100.f, 0.f, 0.f);
					}

					// 서브시스템을 통해 배송 상자 스폰
					AActor* NewBox = DeliverySubsystem->SpawnBox(SpawnInfo.BoxTypeTag, SpawnLocation, SpawnRotation);
					if (NewBox)
					{
						TotalSpawnedCount++;
					}
				}
			}
			GAMERULE_LOG(Log, TEXT("[GameMode] 레벨 스폰 포인트를 활용해 총 %d개의 상자 스폰 완료"), TotalSpawnedCount);
		}
	}
}

void AParcelGameMode::EndRound()
{
	DeliveryRuleComp->EndRound();
}

void AParcelGameMode::OnDeliveryCompleted(APlayerController* Deliverer, int32 ScoreAmount)
{
	DeliveryRuleComp->OnDeliveryCompleted(Deliverer, ScoreAmount);
}

void AParcelGameMode::OnDeliveryFailed(APlayerController* Deliverer)
{
	DeliveryRuleComp->OnDeliveryFailed(Deliverer);
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
