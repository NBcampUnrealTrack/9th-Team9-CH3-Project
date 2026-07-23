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
#include "Core/SessionSubsystem.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UI/ParcelLobbyHUDWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogParcelLobbyAuthority, Log, All);

namespace ParcelLobbyAuthority
{
	const FString LobbyLevelName = TEXT("LV_DF_Lobby_Stage00");
}

AParcelGameMode::AParcelGameMode()
{
	GameStateClass      = AParcelGameState::StaticClass();
	PlayerStateClass    = AParcelPlayerState::StaticClass();
	PlayerControllerClass = AParcelPlayerController::StaticClass();
	DefaultPawnClass    = AParcelCharacter::StaticClass();
	DeliveryRuleComp = CreateDefaultSubobject<UDeliveryRuleComponent>("DeliveryRuleComponent");
	RespawnComp      = CreateDefaultSubobject<URespawnComponent>("RespawnComponent");
	LobbyMapCatalog = TSoftObjectPtr<UDataTable>(
		FSoftObjectPath(TEXT("/Game/Data/DataTables/DT_LobbyMapData.DT_LobbyMapData")));
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

bool AParcelGameMode::RequestLobbyMapSelection(APlayerController* RequestingController, int32 NewMapIndex)
{
	if (!IsAuthorizedLobbyHost(RequestingController))
	{
		UE_LOG(LogParcelLobbyAuthority, Warning, TEXT("Lobby map selection rejected: requester is not the session owner."));
		return false;
	}

	FString ValidatedMapPath;
	if (!ResolveLobbyMapPath(NewMapIndex, ValidatedMapPath))
	{
		UE_LOG(LogParcelLobbyAuthority, Warning, TEXT("Lobby map selection rejected: index %d is not in the server catalog."), NewMapIndex);
		return false;
	}

	AParcelGameState* ParcelGameState = GetWorld() ? GetWorld()->GetGameState<AParcelGameState>() : nullptr;
	if (!ParcelGameState)
	{
		UE_LOG(LogParcelLobbyAuthority, Error, TEXT("Lobby map selection rejected: ParcelGameState is unavailable."));
		return false;
	}

	ParcelGameState->SetSelectedMapIndex(NewMapIndex);
	UE_LOG(LogParcelLobbyAuthority, Log, TEXT("Session owner selected lobby map index %d."), NewMapIndex);
	return true;
}

bool AParcelGameMode::RequestStartLobbyGame(APlayerController* RequestingController)
{
	if (!IsAuthorizedLobbyHost(RequestingController))
	{
		UE_LOG(LogParcelLobbyAuthority, Warning, TEXT("Lobby start rejected: requester is not the session owner."));
		return false;
	}

	const AParcelGameState* ParcelGameState = GetWorld() ? GetWorld()->GetGameState<AParcelGameState>() : nullptr;
	if (!ParcelGameState)
	{
		UE_LOG(LogParcelLobbyAuthority, Error, TEXT("Lobby start rejected: ParcelGameState is unavailable."));
		return false;
	}

	FString SelectedMapPath;
	const int32 SelectedMapIndex = ParcelGameState->GetSelectedMapIndex();
	if (!ResolveLobbyMapPath(SelectedMapIndex, SelectedMapPath))
	{
		UE_LOG(LogParcelLobbyAuthority, Warning, TEXT("Lobby start rejected: selected index %d is not in the server catalog."), SelectedMapIndex);
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	USessionSubsystem* SessionSubsystem = GameInstance ? GameInstance->GetSubsystem<USessionSubsystem>() : nullptr;
	if (!SessionSubsystem)
	{
		UE_LOG(LogParcelLobbyAuthority, Error, TEXT("Lobby start rejected: SessionSubsystem is unavailable."));
		return false;
	}

	if (!SessionSubsystem->TryStartGame(SelectedMapPath))
	{
		UE_LOG(LogParcelLobbyAuthority, Warning, TEXT("Lobby start rejected by the session travel state."));
		return false;
	}

	UE_LOG(LogParcelLobbyAuthority, Log, TEXT("Session owner started catalog map %s (index %d)."), *SelectedMapPath, SelectedMapIndex);
	return true;
}

bool AParcelGameMode::IsAuthorizedLobbyHost(const APlayerController* RequestingController) const
{
	if (!HasAuthority() || !RequestingController || RequestingController->GetWorld() != GetWorld())
	{
		return false;
	}

	if (UGameplayStatics::GetCurrentLevelName(this, true) != ParcelLobbyAuthority::LobbyLevelName)
	{
		return false;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	const USessionSubsystem* SessionSubsystem = GameInstance ? GameInstance->GetSubsystem<USessionSubsystem>() : nullptr;
	return SessionSubsystem && SessionSubsystem->IsSessionOwnerController(RequestingController);
}

bool AParcelGameMode::IsSelectedLobbyMapPath(const FString& CandidateMapPath) const
{
	if (UGameplayStatics::GetCurrentLevelName(this, true) != ParcelLobbyAuthority::LobbyLevelName)
	{
		return false;
	}

	const AParcelGameState* ParcelGameState = GetWorld() ? GetWorld()->GetGameState<AParcelGameState>() : nullptr;
	if (!ParcelGameState)
	{
		return false;
	}

	FString SelectedMapPath;
	return ResolveLobbyMapPath(ParcelGameState->GetSelectedMapIndex(), SelectedMapPath) &&
		SelectedMapPath.Equals(CandidateMapPath, ESearchCase::CaseSensitive);
}

bool AParcelGameMode::ResolveLobbyMapPath(int32 MapIndex, FString& OutMapPath) const
{
	OutMapPath.Reset();

	UDataTable* MapCatalog = LobbyMapCatalog.LoadSynchronous();
	if (!MapCatalog)
	{
		UE_LOG(LogParcelLobbyAuthority, Error, TEXT("Lobby map catalog could not be loaded."));
		return false;
	}

	if (MapCatalog->GetRowStruct() != FParcelMapStageData::StaticStruct())
	{
		UE_LOG(LogParcelLobbyAuthority, Error, TEXT("Lobby map catalog has an unexpected row type."));
		return false;
	}

	TArray<FParcelMapStageData*> MapRows;
	MapCatalog->GetAllRows<FParcelMapStageData>(TEXT("ServerLobbyMapCatalog"), MapRows);
	if (!MapRows.IsValidIndex(MapIndex) || !MapRows[MapIndex])
	{
		return false;
	}

	FString CandidateMapPath = MapRows[MapIndex]->MapPath;
	CandidateMapPath.TrimStartAndEndInline();
	if (CandidateMapPath.IsEmpty() || CandidateMapPath.Contains(TEXT("?")) ||
		!FPackageName::IsValidLongPackageName(CandidateMapPath) ||
		!FPackageName::DoesPackageExist(CandidateMapPath))
	{
		UE_LOG(LogParcelLobbyAuthority, Error, TEXT("Lobby map catalog row %d contains an invalid map package path."), MapIndex);
		return false;
	}

	OutMapPath = MoveTemp(CandidateMapPath);
	return true;
}

// ========================= 컴포넌트 접근 =========================

URespawnComponent* AParcelGameMode::GetRespawnComponent() const
{
	return RespawnComp;
}
