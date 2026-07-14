// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelGameMode.h"

#include "Character/ParcelCharacter.h"
#include "Core/DeliveryRuleComponent.h"
#include "Core/RespawnComponent.h"
#include "Core/ParcelCheatManager.h"
#include "Core/ParcelGameState.h"
#include "Core/ParcelPlayerController.h"
#include "Core/ParcelPlayerState.h"
#include "Delivery/StageData.h"
#include "ParcelLog.h"
#include "Core/ParcelGameInstance.h"
#include "Delivery/StageData.h"
#include "Delivery/DeliverySubsystem.h"
#include "Delivery/DeliveryBoxSpawner.h"
#include "EngineUtils.h"

AParcelGameMode::AParcelGameMode()
{
	GameStateClass      = AParcelGameState::StaticClass();
	PlayerStateClass    = AParcelPlayerState::StaticClass();
	PlayerControllerClass = AParcelPlayerController::StaticClass();
	DefaultPawnClass    = AParcelCharacter::StaticClass();
	DeliveryRuleComp = CreateDefaultSubobject<UDeliveryRuleComponent>("DeliveryRuleComponent");
	RespawnComp      = CreateDefaultSubobject<URespawnComponent>("RespawnComponent");
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

// ========================= 배달 =========================

void AParcelGameMode::OnDeliveryCompleted(APlayerController* Deliverer, int32 ScoreAmount)
{
	DeliveryRuleComp->OnDeliveryCompleted(Deliverer, ScoreAmount);
}

void AParcelGameMode::OnDeliveryFailed(APlayerController* Deliverer, int32 ScoreAmount)
{
	DeliveryRuleComp->OnDeliveryFailed(Deliverer,ScoreAmount);
}

// ========================= 컴포넌트 접근 =========================

URespawnComponent* AParcelGameMode::GetRespawnComponent() const
{
	return RespawnComp;
}
