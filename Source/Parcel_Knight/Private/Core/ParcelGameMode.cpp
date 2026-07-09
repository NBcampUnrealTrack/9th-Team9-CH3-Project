// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelGameMode.h"

#include "Character/ParcelCharacter.h"
#include "Core/DeliveryRuleComponent.h"
#include "Core/ParcelGameState.h"
#include "Core/TeamScoreComponent.h"
#include "Core/ParcelPlayerController.h"
#include "Core/ParcelPlayerState.h"
#include "Core/ParcelGameInstance.h"
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

// ========================= 로드아웃 콘솔 명령어 =========================

void AParcelGameMode::DebugSetLoadoutSlot(int32 SlotIndex, FString ItemTagStr)
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*ItemTagStr), false);
	if (!Tag.IsValid())
	{
		GAMERULE_LOG(Warning, TEXT("[콘솔] 유효하지 않은 태그: %s"), *ItemTagStr);
		return;
	}

	bool bSuccess = GI->SetLoadoutSlot(SlotIndex, Tag);
	GAMERULE_LOG(Log, TEXT("[콘솔] 로드아웃 슬롯[%d] %s → %s"),
		SlotIndex, *ItemTagStr, bSuccess ? TEXT("성공") : TEXT("실패"));
}

void AParcelGameMode::DebugClearLoadoutSlot(int32 SlotIndex)
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	GI->ClearLoadoutSlot(SlotIndex);
	GAMERULE_LOG(Log, TEXT("[콘솔] 로드아웃 슬롯[%d] 초기화"), SlotIndex);
}

void AParcelGameMode::DebugPrintLoadout()
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	const TArray<FGameplayTag>& Loadout = GI->GetLoadout();
	FString Msg = FString::Printf(TEXT("로드아웃 (최대 %d슬롯, 중복허용: %s):"),
		GI->GetMaxLoadoutSlots(), GI->IsAllowDuplicateLoadout() ? TEXT("ON") : TEXT("OFF"));

	for (int32 i = 0; i < Loadout.Num(); ++i)
	{
		FString SlotStr = Loadout[i].IsValid()
			? Loadout[i].ToString()
			: TEXT("(비어있음)");
		Msg += FString::Printf(TEXT("\n  [%d] %s"), i, *SlotStr);
	}

	GAMERULE_LOG(Log, TEXT("[콘솔] %s"), *Msg);
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan, Msg);
}

void AParcelGameMode::DebugSetMaxSlots(int32 Count)
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	GI->SetMaxLoadoutSlots(Count);
	GAMERULE_LOG(Log, TEXT("[콘솔] 최대 슬롯 수 → %d"), Count);
}

void AParcelGameMode::DebugToggleDuplicateLoadout()
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	GI->ToggleDuplicateLoadout();
	GAMERULE_LOG(Log, TEXT("[콘솔] 중복 장착 허용: %s"),
		GI->IsAllowDuplicateLoadout() ? TEXT("ON") : TEXT("OFF"));
}

// ========================= 재화 콘솔 명령어 =========================

// [재화] GameInstance에 위임
void AParcelGameMode::DebugAddMoney(int32 Amount)
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	GI->AddMoney(Amount);
	GAMERULE_LOG(Log, TEXT("[콘솔] 재화 추가: %d → 현재: %d"), Amount, GI->GetMoney());
}

// [재화] 현재 보유 재화를 로그와 화면에 출력
void AParcelGameMode::DebugPrintMoney()
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	FString Msg = FString::Printf(TEXT("보유 재화: %d"), GI->GetMoney());
	GAMERULE_LOG(Log, TEXT("[콘솔] %s"), *Msg);
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Msg);
}
