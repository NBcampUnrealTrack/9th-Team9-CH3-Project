#include "Core/SessionSubsystem.h"

#include "AdvancedFriendsLibrary.h"
#include "Core/ParcelGameInstance.h"
#include "Core/ParcelGameMode.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameMapsSettings.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "Core/ParcelPlayerState.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogParcelSession, Log, All);

namespace ParcelSessionMaps
{
	const FString Lobby = TEXT("/Game/Maps/LV_DF_Lobby_Stage00");
	const FName SteamSubsystem = FName(TEXT("STEAM"));

	FString GetFrontendPackageName()
	{
		const FString ConfiguredMap = UGameMapsSettings::GetGameDefaultMap();
		const FString PackageName = FPackageName::ObjectPathToPackageName(ConfiguredMap);
		return PackageName.IsEmpty() ? ConfiguredMap : PackageName;
	}

	const TCHAR* JoinResultToString(EOnJoinSessionCompleteResult::Type Result)
	{
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::Success: return TEXT("Success");
		case EOnJoinSessionCompleteResult::SessionIsFull: return TEXT("SessionIsFull");
		case EOnJoinSessionCompleteResult::SessionDoesNotExist: return TEXT("SessionDoesNotExist");
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress: return TEXT("CouldNotRetrieveAddress");
		case EOnJoinSessionCompleteResult::AlreadyInSession: return TEXT("AlreadyInSession");
		default: return TEXT("UnknownError");
		}
	}
}

namespace ParcelSessionSettings
{
	const FName ParcelGameKey(TEXT("PARCEL_GAME"));
	const FString ParcelGameValue(TEXT("ParcelKnight"));
	constexpr int32 MaxSearchResults = 100;
}

void USessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&USessionSubsystem::HandlePostLoadMap);

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(
			this,
			&USessionSubsystem::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(
			this,
			&USessionSubsystem::HandleTravelFailure);
	}
}

void USessionSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	if (GEngine)
	{
		if (NetworkFailureHandle.IsValid())
		{
			GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
			NetworkFailureHandle.Reset();
		}
		if (TravelFailureHandle.IsValid())
		{
			GEngine->OnTravelFailure().Remove(TravelFailureHandle);
			TravelFailureHandle.Reset();
		}
	}

	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid() && CurrentOperation == ESessionOperation::Finding)
	{
		Sessions->CancelFindSessions();
	}
	ClearAllDelegateHandles(Sessions);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OperationTimeoutHandle);
	}

	SessionSearch.Reset();
	PendingJoinResult = FOnlineSessionSearchResult();
	bHasPendingJoinResult = false;
	PendingNumConnections = 0;
	DestroyIntent = EDestroyIntent::None;
	CurrentOperation = ESessionOperation::None;
	bOperationFailureReported = false;
	bLeaveInProgress = false;
	bHostTravelInProgress = false;
	bClientTravelInProgress = false;

	Super::Deinitialize();
}

IOnlineSessionPtr USessionSubsystem::GetSessionInterface() const
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	return OSS ? OSS->GetSessionInterface() : nullptr;
}

bool USessionSubsystem::GetSteamSessionInterface(
	IOnlineSessionPtr& OutSessions,
	const TCHAR* Context) const
{
	OutSessions.Reset();

	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS)
	{
		UE_LOG(LogParcelSession, Error, TEXT("[%s] OnlineSubsystem is unavailable."), Context);
		return false;
	}

	if (OSS->GetSubsystemName() != ParcelSessionMaps::SteamSubsystem)
	{
		UE_LOG(
			LogParcelSession,
			Error,
			TEXT("[%s] Steam is required, but the active OnlineSubsystem is %s."),
			Context,
			*OSS->GetSubsystemName().ToString());
		return false;
	}

	const IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
	if (!Identity.IsValid() || Identity->GetLoginStatus(0) != ELoginStatus::LoggedIn)
	{
		UE_LOG(LogParcelSession, Error, TEXT("[%s] Local Steam user 0 is not logged in."), Context);
		return false;
	}

	OutSessions = OSS->GetSessionInterface();
	if (!OutSessions.IsValid())
	{
		UE_LOG(LogParcelSession, Error, TEXT("[%s] Steam session interface is unavailable."), Context);
		return false;
	}

	return true;
}

void USessionSubsystem::BeginOperation(ESessionOperation NewOperation)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OperationTimeoutHandle);
		World->GetTimerManager().SetTimer(
			OperationTimeoutHandle,
			this,
			&USessionSubsystem::OnOperationTimeout,
			OperationTimeoutSeconds,
			false);
	}

	CurrentOperation = NewOperation;
	bOperationFailureReported = false;
}

void USessionSubsystem::ClearOperationState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OperationTimeoutHandle);
	}

	CurrentOperation = ESessionOperation::None;
	bOperationFailureReported = false;
}

void USessionSubsystem::ClearDelegateHandleForOperation(
	const IOnlineSessionPtr& Sessions,
	ESessionOperation Operation)
{
	if (Sessions.IsValid())
	{
		switch (Operation)
		{
		case ESessionOperation::Creating:
			if (CreateSessionHandle.IsValid())
			{
				Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
			}
			break;
		case ESessionOperation::Starting:
			if (StartSessionHandle.IsValid())
			{
				Sessions->ClearOnStartSessionCompleteDelegate_Handle(StartSessionHandle);
			}
			break;
		case ESessionOperation::Finding:
			if (FindSessionsHandle.IsValid())
			{
				Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
			}
			break;
		case ESessionOperation::Joining:
			if (JoinSessionHandle.IsValid())
			{
				Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
			}
			break;
		case ESessionOperation::Destroying:
			if (DestroySessionHandle.IsValid())
			{
				Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
			}
			break;
		default:
			break;
		}
	}

	switch (Operation)
	{
	case ESessionOperation::Creating: CreateSessionHandle.Reset(); break;
	case ESessionOperation::Starting: StartSessionHandle.Reset(); break;
	case ESessionOperation::Finding: FindSessionsHandle.Reset(); break;
	case ESessionOperation::Joining: JoinSessionHandle.Reset(); break;
	case ESessionOperation::Destroying: DestroySessionHandle.Reset(); break;
	default: break;
	}
}

void USessionSubsystem::ClearAllDelegateHandles(const IOnlineSessionPtr& Sessions)
{
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Creating);
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Starting);
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Finding);
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Joining);
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Destroying);
}

void USessionSubsystem::ReportSessionStatus(const FString& Message, bool bIsError)
{
	if (bIsError)
	{
		UE_LOG(LogParcelSession, Error, TEXT("%s"), *Message);
	}
	else
	{
		UE_LOG(LogParcelSession, Log, TEXT("%s"), *Message);
	}

	OnSessionStatusMessage.Broadcast(FText::FromString(Message), bIsError);
}

void USessionSubsystem::ReportJoinFailure(const FString& Reason)
{
	ReportSessionStatus(FString::Printf(TEXT("JoinSession failed: %s"), *Reason), true);
}

void USessionSubsystem::ReportOperationFailure(ESessionOperation Operation)
{
	if (bOperationFailureReported)
	{
		return;
	}

	bOperationFailureReported = true;

	switch (Operation)
	{
	case ESessionOperation::Creating:
	case ESessionOperation::Starting:
		OnSessionCreateComplete.Broadcast(false);
		break;
	case ESessionOperation::Finding:
		OnSessionFindComplete.Broadcast(false);
		break;
	case ESessionOperation::Joining:
		ReportJoinFailure(TEXT("the online request timed out"));
		OnSessionJoinComplete.Broadcast(false);
		break;
	case ESessionOperation::Destroying:
		switch (DestroyIntent)
		{
		case EDestroyIntent::Recreate:
			OnSessionCreateComplete.Broadcast(false);
			break;
		case EDestroyIntent::JoinPending:
			ReportJoinFailure(TEXT("the previous GameSession could not be destroyed before joining"));
			OnSessionJoinComplete.Broadcast(false);
			break;
		case EDestroyIntent::UserRequested:
		case EDestroyIntent::Leave:
			OnSessionDestroyComplete.Broadcast(false);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

void USessionSubsystem::OnOperationTimeout()
{
	if (CurrentOperation == ESessionOperation::None)
	{
		return;
	}

	UE_LOG(
		LogParcelSession,
		Error,
		TEXT("Session operation %d timed out after %.0f seconds."),
		static_cast<uint8>(CurrentOperation),
		OperationTimeoutSeconds);

	if (CurrentOperation == ESessionOperation::Finding)
	{
		const IOnlineSessionPtr Sessions = GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->CancelFindSessions();
		}
		ClearDelegateHandleForOperation(Sessions, ESessionOperation::Finding);
		ClearOperationState();
		SessionSearch.Reset();
		ReportSessionStatus(TEXT("FindSessions failed: the Steam session search timed out."), true);
		OnSessionFindComplete.Broadcast(false);
		return;
	}

	if (CurrentOperation == ESessionOperation::Destroying && DestroyIntent == EDestroyIntent::Leave)
	{
		ReportOperationFailure(ESessionOperation::Destroying);
		// DestroySession cannot be cancelled. Return the user to the front-end,
		// but keep this operation quarantined until its callback is drained so a
		// late Steam task cannot remove a newly created NAME_GameSession.
		TravelToFrontend(false);
		return;
	}

	// Create/Start/Join/Destroy have no backend cancellation API. Keep their
	// delegate and state until the late callback arrives so it cannot satisfy a
	// newly registered operation of the same type.
	ReportOperationFailure(CurrentOperation);
}

void USessionSubsystem::CreateSession(int32 NumPublicConnections)
{
	if (IsSessionTransitionLocked())
	{
		UE_LOG(LogParcelSession, Warning, TEXT("CreateSession ignored during map or leave transition."));
		return;
	}

	if (NumPublicConnections < 1)
	{
		UE_LOG(LogParcelSession, Error, TEXT("CreateSession requires at least one public connection."));
		OnSessionCreateComplete.Broadcast(false);
		return;
	}

	if (CurrentOperation != ESessionOperation::None)
	{
		UE_LOG(LogParcelSession, Warning, TEXT("CreateSession ignored because another session operation is active."));
		return;
	}

	IOnlineSessionPtr Sessions;
	if (!GetSteamSessionInterface(Sessions, TEXT("CreateSession")))
	{
		OnSessionCreateComplete.Broadcast(false);
		return;
	}

	// Hosting always enters the Lobby first. Stage travel remains StartGame's job.
	if (UParcelGameInstance* GI = Cast<UParcelGameInstance>(GetGameInstance()))
	{
		GI->SetPendingMapPath(ParcelSessionMaps::Lobby);
	}

	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		PendingNumConnections = NumPublicConnections;
		BeginDestroySession(EDestroyIntent::Recreate);
		return;
	}

	BeginCreateSession(NumPublicConnections);
}

bool USessionSubsystem::BeginCreateSession(int32 NumPublicConnections)
{
	if (CurrentOperation != ESessionOperation::None)
	{
		return false;
	}

	IOnlineSessionPtr Sessions;
	if (!GetSteamSessionInterface(Sessions, TEXT("BeginCreateSession")))
	{
		OnSessionCreateComplete.Broadcast(false);
		return false;
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.NumPublicConnections = NumPublicConnections;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowInvites = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.Set(
		ParcelSessionSettings::ParcelGameKey,
		ParcelSessionSettings::ParcelGameValue,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	BeginOperation(ESessionOperation::Creating);
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Creating);
	CreateSessionHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(
			this,
			&USessionSubsystem::OnCreateSessionComplete));

	UE_LOG(
		LogParcelSession,
		Log,
		TEXT("Creating Steam GameSession for %d public connections with PARCEL_GAME=\"%s\"."),
		NumPublicConnections,
		*ParcelSessionSettings::ParcelGameValue);
	const bool bStarted = Sessions->CreateSession(0, NAME_GameSession, SessionSettings);
	if (!bStarted && CurrentOperation == ESessionOperation::Creating)
	{
		ClearDelegateHandleForOperation(Sessions, ESessionOperation::Creating);
		ClearOperationState();
		UE_LOG(LogParcelSession, Error, TEXT("CreateSession returned false immediately."));
		OnSessionCreateComplete.Broadcast(false);
		return false;
	}

	return bStarted;
}

void USessionSubsystem::FindSessions()
{
	if (IsSessionTransitionLocked())
	{
		UE_LOG(LogParcelSession, Warning, TEXT("FindSessions ignored during map or leave transition."));
		return;
	}

	if (CurrentOperation != ESessionOperation::None)
	{
		UE_LOG(LogParcelSession, Warning, TEXT("FindSessions ignored because another session operation is active."));
		if (CurrentOperation != ESessionOperation::Finding)
		{
			OnSessionFindComplete.Broadcast(false);
		}
		return;
	}

	// The room list must never display or recreate entries from a previous request.
	SessionSearch.Reset();
	OnSessionSearchStarted.Broadcast();

	IOnlineSessionPtr Sessions;
	if (!GetSteamSessionInterface(Sessions, TEXT("FindSessions")))
	{
		ReportSessionStatus(
			TEXT("FindSessions failed: Steam is unavailable or the local Steam user is not logged in."),
			true);
		OnSessionFindComplete.Broadcast(false);
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = ParcelSessionSettings::MaxSearchResults;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->TimeoutInSeconds = 10.0f;
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(
		ParcelSessionSettings::ParcelGameKey,
		ParcelSessionSettings::ParcelGameValue,
		EOnlineComparisonOp::Equals);

	BeginOperation(ESessionOperation::Finding);
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Finding);
	FindSessionsHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(
			this,
			&USessionSubsystem::OnFindSessionsComplete));

	const bool bStarted = Sessions->FindSessions(0, SessionSearch.ToSharedRef());
	if (!bStarted && CurrentOperation == ESessionOperation::Finding)
	{
		ClearDelegateHandleForOperation(Sessions, ESessionOperation::Finding);
		ClearOperationState();
		SessionSearch.Reset();
		UE_LOG(LogParcelSession, Error, TEXT("FindSessions returned false immediately."));
		ReportSessionStatus(
			TEXT("FindSessions failed: the online subsystem rejected the search request immediately."),
			true);
		OnSessionFindComplete.Broadcast(false);
	}
}

void USessionSubsystem::JoinSession(int32 SessionIndex)
{
	if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		UE_LOG(LogParcelSession, Error, TEXT("JoinSession received invalid search-result index %d."), SessionIndex);
		ReportJoinFailure(TEXT("the selected session index is no longer valid; refresh the room list"));
		OnSessionJoinComplete.Broadcast(false);
		return;
	}

	StartJoinSession(SessionSearch->SearchResults[SessionIndex]);
}

bool USessionSubsystem::JoinSessionResult(const FOnlineSessionSearchResult& SessionResult)
{
	return StartJoinSession(SessionResult);
}

void USessionSubsystem::NotifySessionInviteAccepted(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		ReportSessionStatus(TEXT("Steam session invite accepted. Joining the invited GameSession."), false);
	}
	else
	{
		ReportSessionStatus(
			TEXT("Steam session invite acceptance failed because the invite result was invalid."),
			true);
	}
}

bool USessionSubsystem::StartJoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (IsSessionTransitionLocked())
	{
		UE_LOG(LogParcelSession, Warning, TEXT("JoinSession rejected during map or leave transition."));
		ReportJoinFailure(TEXT("a map or leave transition is already in progress"));
		OnSessionJoinComplete.Broadcast(false);
		return false;
	}

	if (!SessionResult.IsValid())
	{
		UE_LOG(LogParcelSession, Error, TEXT("JoinSession received an invalid session result."));
		ReportJoinFailure(TEXT("the session search result is invalid"));
		OnSessionJoinComplete.Broadcast(false);
		return false;
	}

	IOnlineSessionPtr Sessions;
	if (!GetSteamSessionInterface(Sessions, TEXT("JoinSession")))
	{
		ReportJoinFailure(TEXT("Steam is unavailable or the local Steam user is not logged in"));
		OnSessionJoinComplete.Broadcast(false);
		return false;
	}

	bool bCancelledActiveFind = false;
	if (CurrentOperation == ESessionOperation::Finding)
	{
		Sessions->CancelFindSessions();
		ClearDelegateHandleForOperation(Sessions, ESessionOperation::Finding);
		ClearOperationState();
		bCancelledActiveFind = true;
	}

	if (CurrentOperation == ESessionOperation::Destroying)
	{
		if (bOperationFailureReported || DestroyIntent == EDestroyIntent::Leave)
		{
			UE_LOG(LogParcelSession, Warning, TEXT("Invite join rejected while a timed-out or leave destroy is active."));
			ReportJoinFailure(TEXT("the current GameSession is still leaving or timed out while being destroyed"));
			OnSessionJoinComplete.Broadcast(false);
			return false;
		}

		PendingJoinResult = SessionResult;
		bHasPendingJoinResult = true;
		PendingNumConnections = 0;
		DestroyIntent = EDestroyIntent::JoinPending;
		ReportSessionStatus(TEXT("Joining session..."), false);
		UE_LOG(LogParcelSession, Log, TEXT("Session join queued behind the active GameSession destroy."));
		if (bCancelledActiveFind)
		{
			OnSessionFindComplete.Broadcast(false);
		}
		return true;
	}

	if (CurrentOperation != ESessionOperation::None)
	{
		UE_LOG(LogParcelSession, Warning, TEXT("JoinSession rejected because another session operation is active."));
		ReportJoinFailure(TEXT("another session operation is already active"));
		OnSessionJoinComplete.Broadcast(false);
		return false;
	}

	ReportSessionStatus(TEXT("Joining session..."), false);

	bool bJoinStarted = false;
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		PendingJoinResult = SessionResult;
		bHasPendingJoinResult = true;
		bJoinStarted = BeginDestroySession(EDestroyIntent::JoinPending);
	}
	else
	{
		bJoinStarted = BeginJoinSession(SessionResult);
	}

	if (bCancelledActiveFind)
	{
		OnSessionFindComplete.Broadcast(false);
	}
	return bJoinStarted;
}

bool USessionSubsystem::BeginJoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (CurrentOperation != ESessionOperation::None)
	{
		return false;
	}

	IOnlineSessionPtr Sessions;
	if (!GetSteamSessionInterface(Sessions, TEXT("BeginJoinSession")))
	{
		ReportJoinFailure(TEXT("Steam is unavailable or the local Steam user is not logged in"));
		OnSessionJoinComplete.Broadcast(false);
		return false;
	}

	BeginOperation(ESessionOperation::Joining);
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Joining);
	JoinSessionHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(
			this,
			&USessionSubsystem::OnJoinSessionComplete));

	const bool bUsesPresenceBefore = SessionResult.Session.SessionSettings.bUsesPresence;
	const bool bUsesLobbiesBefore = SessionResult.Session.SessionSettings.bUseLobbiesIfAvailable;
	FOnlineSessionSearchResult SessionToJoin = SessionResult;
	SessionToJoin.Session.SessionSettings.bUsesPresence = true;
	SessionToJoin.Session.SessionSettings.bUseLobbiesIfAvailable = true;

	UE_LOG(
		LogParcelSession,
		Log,
		TEXT("Joining Steam GameSession: SessionId=%s bUsesPresence=%d->%d bUseLobbiesIfAvailable=%d->%d."),
		*SessionToJoin.GetSessionIdStr(),
		bUsesPresenceBefore,
		SessionToJoin.Session.SessionSettings.bUsesPresence,
		bUsesLobbiesBefore,
		SessionToJoin.Session.SessionSettings.bUseLobbiesIfAvailable);
	const bool bStarted = Sessions->JoinSession(0, NAME_GameSession, SessionToJoin);
	if (!bStarted && CurrentOperation == ESessionOperation::Joining)
	{
		ClearDelegateHandleForOperation(Sessions, ESessionOperation::Joining);
		ClearOperationState();
		UE_LOG(LogParcelSession, Error, TEXT("JoinSession returned false immediately."));
		ReportJoinFailure(TEXT("the online subsystem rejected the join request immediately"));
		OnSessionJoinComplete.Broadcast(false);
		CleanupNamedSession();
		return false;
	}

	return bStarted;
}

void USessionSubsystem::DestroySession()
{
	if (IsSessionTransitionLocked())
	{
		UE_LOG(LogParcelSession, Warning, TEXT("DestroySession ignored during map or leave transition."));
		return;
	}

	if (CurrentOperation != ESessionOperation::None)
	{
		UE_LOG(LogParcelSession, Warning, TEXT("DestroySession rejected because another session operation is active."));
		OnSessionDestroyComplete.Broadcast(false);
		return;
	}

	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !Sessions->GetNamedSession(NAME_GameSession))
	{
		OnSessionDestroyComplete.Broadcast(true);
		return;
	}

	BeginDestroySession(EDestroyIntent::UserRequested);
}

bool USessionSubsystem::BeginDestroySession(EDestroyIntent Intent)
{
	if (CurrentOperation != ESessionOperation::None)
	{
		return false;
	}

	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !Sessions->GetNamedSession(NAME_GameSession))
	{
		CompleteDestroyIntent(Intent, Sessions.IsValid());
		return Sessions.IsValid();
	}

	DestroyIntent = Intent;
	BeginOperation(ESessionOperation::Destroying);
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Destroying);
	DestroySessionHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(
			this,
			&USessionSubsystem::OnDestroySessionComplete));

	UE_LOG(LogParcelSession, Log, TEXT("Destroying GameSession with intent %d."), static_cast<uint8>(Intent));
	const bool bStarted = Sessions->DestroySession(NAME_GameSession);
	if (!bStarted && CurrentOperation == ESessionOperation::Destroying && DestroyIntent == Intent)
	{
		ClearDelegateHandleForOperation(Sessions, ESessionOperation::Destroying);
		ClearOperationState();
		DestroyIntent = EDestroyIntent::None;
		UE_LOG(LogParcelSession, Error, TEXT("DestroySession returned false immediately."));
		CompleteDestroyIntent(Intent, false);
		return false;
	}

	return bStarted;
}

void USessionSubsystem::CompleteDestroyIntent(EDestroyIntent Intent, bool bWasSuccessful)
{
	switch (Intent)
	{
	case EDestroyIntent::UserRequested:
		OnSessionDestroyComplete.Broadcast(bWasSuccessful);
		break;

	case EDestroyIntent::Recreate:
	{
		const int32 NumConnections = PendingNumConnections;
		PendingNumConnections = 0;
		if (bWasSuccessful)
		{
			BeginCreateSession(NumConnections);
		}
		else
		{
			UE_LOG(LogParcelSession, Error, TEXT("Recreate aborted because the existing session could not be destroyed."));
			OnSessionCreateComplete.Broadcast(false);
		}
		break;
	}

	case EDestroyIntent::JoinPending:
	{
		const FOnlineSessionSearchResult SessionToJoin = PendingJoinResult;
		const bool bHadPendingJoin = bHasPendingJoinResult;
		PendingJoinResult = FOnlineSessionSearchResult();
		bHasPendingJoinResult = false;
		if (bWasSuccessful && bHadPendingJoin && SessionToJoin.IsValid())
		{
			BeginJoinSession(SessionToJoin);
		}
		else
		{
			UE_LOG(LogParcelSession, Error, TEXT("Pending join aborted because the previous GameSession could not be destroyed."));
			ReportJoinFailure(TEXT("the previous GameSession could not be destroyed"));
			OnSessionJoinComplete.Broadcast(false);
		}
		break;
	}

	case EDestroyIntent::Leave:
	{
		if (!bWasSuccessful)
		{
			UE_LOG(
				LogParcelSession,
				Error,
				TEXT("Leave reached the front-end, but Steam did not confirm GameSession destruction. A new host/join will retry destroy first."));
		}
		PendingNumConnections = 0;
		PendingJoinResult = FOnlineSessionSearchResult();
		bHasPendingJoinResult = false;
		OnSessionDestroyComplete.Broadcast(bWasSuccessful);
		TravelToFrontend();
		break;
	}

	case EDestroyIntent::Cleanup:
		if (!bWasSuccessful)
		{
			UE_LOG(LogParcelSession, Warning, TEXT("Failed to clean up a stale GameSession."));
		}
		break;

	default:
		break;
	}
}

void USessionSubsystem::CleanupNamedSession()
{
	if (CurrentOperation != ESessionOperation::None)
	{
		return;
	}

	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession))
	{
		BeginDestroySession(EDestroyIntent::Cleanup);
	}
}

void USessionSubsystem::LeaveSession()
{
	if (bLeaveInProgress)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogParcelSession, Error, TEXT("LeaveSession failed because there is no world."));
		return;
	}

	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (CurrentOperation == ESessionOperation::None &&
		(!Sessions.IsValid() || !Sessions->GetNamedSession(NAME_GameSession)) &&
		IsFrontendWorld(World))
	{
		return;
	}

	bLeaveInProgress = true;
	bHostTravelInProgress = false;
	bClientTravelInProgress = false;

	// Notify every remote client first. Their ClientReturnToMainMenu RPC calls
	// UParcelGameInstance::ReturnToMainMenu, which routes back to this subsystem
	// on each machine. The local host re-entry is stopped by bLeaveInProgress.
	if (World->GetNetMode() == NM_ListenServer)
	{
		if (AGameModeBase* GameMode = World->GetAuthGameMode())
		{
			GameMode->ReturnToMainMenuHost();
		}
	}

	if (CurrentOperation == ESessionOperation::Destroying)
	{
		if (bOperationFailureReported)
		{
			DestroyIntent = EDestroyIntent::Leave;
			TravelToFrontend(false);
			return;
		}

		DestroyIntent = EDestroyIntent::Leave;
		PendingNumConnections = 0;
		PendingJoinResult = FOnlineSessionSearchResult();
		bHasPendingJoinResult = false;
		return;
	}

	if (CurrentOperation == ESessionOperation::Finding)
	{
		if (Sessions.IsValid())
		{
			Sessions->CancelFindSessions();
		}
		ClearDelegateHandleForOperation(Sessions, ESessionOperation::Finding);
		ClearOperationState();
		OnSessionFindComplete.Broadcast(false);
	}
	else if (CurrentOperation != ESessionOperation::None)
	{
		// Create/Start/Join cannot be cancelled safely. Keep the operation and its
		// delegate alive; its completion callback will convert the result into a
		// Leave destroy instead of starting travel or reporting success.
		UE_LOG(LogParcelSession, Log, TEXT("Leave queued behind active operation %d."), static_cast<uint8>(CurrentOperation));
		if (bOperationFailureReported)
		{
			TravelToFrontend(false);
		}
		return;
	}

	BeginDestroySession(EDestroyIntent::Leave);
}

void USessionSubsystem::TravelToFrontend(bool bFinishLeaveFlow)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		bLeaveInProgress = false;
		return;
	}

	if (IsFrontendWorld(World))
	{
		if (bFinishLeaveFlow)
		{
			bLeaveInProgress = false;
		}
		return;
	}

	const FString FrontendPackageName = ParcelSessionMaps::GetFrontendPackageName();
	if (FrontendPackageName.IsEmpty())
	{
		UE_LOG(LogParcelSession, Error, TEXT("Cannot return to the front-end because GameDefaultMap is empty."));
		bLeaveInProgress = false;
		return;
	}

	UE_LOG(LogParcelSession, Log, TEXT("Returning to configured front-end map %s."), *FrontendPackageName);
	UGameplayStatics::OpenLevel(World, FName(*FrontendPackageName));
}

bool USessionSubsystem::IsFrontendWorld(const UWorld* World) const
{
	return World &&
		UGameplayStatics::GetCurrentLevelName(World, true) ==
		FPackageName::GetShortName(ParcelSessionMaps::GetFrontendPackageName());
}

bool USessionSubsystem::IsSessionTransitionLocked() const
{
	return bLeaveInProgress || bHostTravelInProgress || bClientTravelInProgress;
}

void USessionSubsystem::StartGame(const FString& MapPath)
{
	TryStartGame(MapPath);
}

bool USessionSubsystem::TryStartGame(const FString& MapPath)
{
	if (IsSessionTransitionLocked())
	{
		UE_LOG(LogParcelSession, Warning, TEXT("StartGame ignored during map or leave transition."));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client || MapPath.IsEmpty())
	{
		UE_LOG(LogParcelSession, Error, TEXT("StartGame is host-only and requires a valid map path."));
		return false;
	}

	const AParcelGameMode* ParcelGameMode = World->GetAuthGameMode<AParcelGameMode>();
	if (!ParcelGameMode || !ParcelGameMode->IsSelectedLobbyMapPath(MapPath))
	{
		UE_LOG(LogParcelSession, Warning, TEXT("StartGame rejected a path that is not the server-selected lobby catalog map."));
		return false;
	}

	if (CurrentOperation != ESessionOperation::None)
	{
		UE_LOG(LogParcelSession, Warning, TEXT("StartGame ignored while a session operation is active."));
		return false;
	}

	const FString TravelURL = MapPath + TEXT("?listen");
	bHostTravelInProgress = true;
	if (!World->ServerTravel(TravelURL))
	{
		bHostTravelInProgress = false;
		UE_LOG(LogParcelSession, Error, TEXT("ServerTravel failed to start for %s."), *TravelURL);
		return false;
	}

	return true;
}

int32 USessionSubsystem::GetSearchResultCount() const
{
	return SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : 0;
}

FString USessionSubsystem::GetSessionOwnerName(int32 Index) const
{
	if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(Index))
	{
		return TEXT("");
	}
	return SessionSearch->SearchResults[Index].Session.OwningUserName;
}

int32 USessionSubsystem::GetSessionPlayerCount(int32 Index) const
{
	if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(Index))
	{
		return 0;
	}

	const FOnlineSession& Session = SessionSearch->SearchResults[Index].Session;
	return Session.SessionSettings.NumPublicConnections - Session.NumOpenPublicConnections;
}

bool USessionSubsystem::CanInviteToCurrentSession() const
{
	if (IsSessionTransitionLocked())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return false;
	}

	IOnlineSessionPtr Sessions;
	if (!GetSteamSessionInterface(Sessions, TEXT("CanInviteToCurrentSession")))
	{
		return false;
	}

	FNamedOnlineSession* NamedSession = Sessions->GetNamedSession(NAME_GameSession);
	if (!NamedSession || !NamedSession->SessionSettings.bAllowInvites)
	{
		return false;
	}

	const EOnlineSessionState::Type State = Sessions->GetSessionState(NAME_GameSession);
	if (State == EOnlineSessionState::NoSession || State == EOnlineSessionState::Destroying)
	{
		return false;
	}

	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	const IOnlineIdentityPtr Identity = OSS ? OSS->GetIdentityInterface() : nullptr;
	const TSharedPtr<const FUniqueNetId> LocalUserId =
		Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
	if (LocalUserId.IsValid() && NamedSession->OwningUserId.IsValid())
	{
		return *LocalUserId == *NamedSession->OwningUserId;
	}

	return World->GetNetMode() == NM_ListenServer;
}

bool USessionSubsystem::IsSessionOwnerController(const APlayerController* PlayerController) const
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client || !PlayerController || PlayerController->GetWorld() != World)
	{
		return false;
	}

	// [Fix 1] ListenServer 환경의 로컬 방장 컨트롤러이면 무조건 세션 소유자로 인정 (PIE/LAN 테스트 완벽 보장)
	if (World->GetNetMode() == NM_ListenServer && PlayerController->IsLocalController())
	{
		return true;
	}

	// [Fix 2] PostLogin 시점에 검증 및 복제된 ParcelPlayerState의 IsHostPlayer() 확인
	const AParcelPlayerState* ParcelPS = PlayerController->GetPlayerState<AParcelPlayerState>();
	if (ParcelPS && ParcelPS->IsHostPlayer())
	{
		return true;
	}

	// [기존 로직] Steam OnlineSubsystem UniqueNetId 검증 (실제 패키징/스팀 빌드용)
	const TSharedPtr<const FUniqueNetId> RequestingUserId =
	   ParcelPS ? ParcelPS->GetUniqueId().GetUniqueNetId() : nullptr;
	if (!RequestingUserId.IsValid())
	{
		return false;
	}

	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		return false;
	}

	const FNamedOnlineSession* NamedSession = Sessions->GetNamedSession(NAME_GameSession);
	return NamedSession && NamedSession->OwningUserId.IsValid() &&
		*RequestingUserId == *NamedSession->OwningUserId;
}

bool USessionSubsystem::SendSessionInviteToFriend(
	APlayerController* PlayerController,
	const FBPUniqueNetId& FriendUniqueNetId) const
{
	if (!PlayerController || !PlayerController->IsLocalController() ||
		!FriendUniqueNetId.IsValid() || !CanInviteToCurrentSession())
	{
		UE_LOG(LogParcelSession, Warning, TEXT("Steam session invite rejected by local validation."));
		return false;
	}

	EBlueprintResultSwitch Result = EBlueprintResultSwitch::OnFailure;
	UAdvancedFriendsLibrary::SendSessionInviteToFriend(PlayerController, FriendUniqueNetId, Result);
	const bool bRequested = Result == EBlueprintResultSwitch::OnSuccess;
	UE_LOG(LogParcelSession, Log, TEXT("Steam invite request result: %s."), bRequested ? TEXT("success") : TEXT("failure"));
	return bRequested;
}

void USessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionName != NAME_GameSession || CurrentOperation != ESessionOperation::Creating)
	{
		return;
	}

	const IOnlineSessionPtr Sessions = GetSessionInterface();
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Creating);
	const bool bTimedOut = bOperationFailureReported;
	ClearOperationState();
	if (bLeaveInProgress)
	{
		UE_LOG(LogParcelSession, Log, TEXT("CreateSession completion converted to pending Leave."));
		BeginDestroySession(EDestroyIntent::Leave);
		return;
	}

	if (bTimedOut)
	{
		UE_LOG(LogParcelSession, Warning, TEXT("Ignoring late CreateSession completion after timeout."));
		if (bWasSuccessful)
		{
			CleanupNamedSession();
		}
		return;
	}

	if (!bWasSuccessful || !Sessions.IsValid())
	{
		UE_LOG(LogParcelSession, Error, TEXT("CreateSession completion failed."));
		OnSessionCreateComplete.Broadcast(false);
		return;
	}

	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogParcelSession, Error, TEXT("CreateSession completed on an invalid host world."));
		OnSessionCreateComplete.Broadcast(false);
		CleanupNamedSession();
		return;
	}

	BeginOperation(ESessionOperation::Starting);
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Starting);
	StartSessionHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(
		FOnStartSessionCompleteDelegate::CreateUObject(
			this,
			&USessionSubsystem::OnStartSessionComplete));

	const bool bStarted = Sessions->StartSession(NAME_GameSession);
	if (!bStarted && CurrentOperation == ESessionOperation::Starting)
	{
		ClearDelegateHandleForOperation(Sessions, ESessionOperation::Starting);
		ClearOperationState();
		UE_LOG(LogParcelSession, Error, TEXT("StartSession returned false immediately."));
		OnSessionCreateComplete.Broadcast(false);
		CleanupNamedSession();
	}
}

void USessionSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionName != NAME_GameSession || CurrentOperation != ESessionOperation::Starting)
	{
		return;
	}

	const IOnlineSessionPtr Sessions = GetSessionInterface();
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Starting);
	const bool bTimedOut = bOperationFailureReported;
	ClearOperationState();
	if (bLeaveInProgress)
	{
		UE_LOG(LogParcelSession, Log, TEXT("StartSession completion converted to pending Leave."));
		BeginDestroySession(EDestroyIntent::Leave);
		return;
	}

	if (bTimedOut)
	{
		UE_LOG(LogParcelSession, Warning, TEXT("Ignoring late StartSession completion after timeout."));
		CleanupNamedSession();
		return;
	}

	if (!bWasSuccessful || !Sessions.IsValid())
	{
		UE_LOG(LogParcelSession, Error, TEXT("StartSession completion failed."));
		OnSessionCreateComplete.Broadcast(false);
		CleanupNamedSession();
		return;
	}

	UWorld* World = GetWorld();
	const FString TravelURL = ParcelSessionMaps::Lobby + TEXT("?listen");
	bHostTravelInProgress = true;
	if (!World || World->GetNetMode() == NM_Client || !World->ServerTravel(TravelURL))
	{
		bHostTravelInProgress = false;
		UE_LOG(LogParcelSession, Error, TEXT("Lobby ServerTravel failed to start for %s."), *TravelURL);
		OnSessionCreateComplete.Broadcast(false);
		CleanupNamedSession();
		return;
	}

	UE_LOG(LogParcelSession, Log, TEXT("GameSession started; Lobby listen travel issued."));
	OnSessionCreateComplete.Broadcast(true);
}

void USessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (CurrentOperation != ESessionOperation::Finding)
	{
		return;
	}

	const IOnlineSessionPtr Sessions = GetSessionInterface();
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Finding);
	ClearOperationState();
	const bool bCompletedSuccessfully = bWasSuccessful && Sessions.IsValid();

	if (SessionSearch.IsValid())
	{
		TArray<FOnlineSessionSearchResult> FilteredResults;
		if (bCompletedSuccessfully)
		{
			FilteredResults.Reserve(SessionSearch->SearchResults.Num());
			for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
			{
				FString ParcelGameValue;
				const bool bHasParcelGameValue = SearchResult.Session.SessionSettings.Get(
					ParcelSessionSettings::ParcelGameKey,
					ParcelGameValue);
				const bool bIsParcelSession = bHasParcelGameValue
					&& ParcelGameValue == ParcelSessionSettings::ParcelGameValue;
				const bool bHasOpenPublicSlot = SearchResult.Session.NumOpenPublicConnections > 0;

				if (!SearchResult.IsValid() || !bIsParcelSession || !bHasOpenPublicSlot)
				{
					UE_LOG(
						LogParcelSession,
						Verbose,
						TEXT("Filtered session result: Valid=%d SessionId=%s ParcelGameValue=\"%s\" OpenPublicConnections=%d."),
						SearchResult.IsValid(),
						*SearchResult.GetSessionIdStr(),
						*ParcelGameValue,
						SearchResult.Session.NumOpenPublicConnections);
					continue;
				}

				FilteredResults.Add(SearchResult);
			}
		}

		SessionSearch->SearchResults = MoveTemp(FilteredResults);
	}

	const int32 FilteredResultCount = GetSearchResultCount();
	UE_LOG(
		LogParcelSession,
		Log,
		TEXT("FindSessions completed: success=%d filteredResults=%d."),
		bCompletedSuccessfully,
		FilteredResultCount);

	if (bCompletedSuccessfully && SessionSearch.IsValid())
	{
		for (int32 Index = 0; Index < SessionSearch->SearchResults.Num(); ++Index)
		{
			const FOnlineSessionSearchResult& SearchResult = SessionSearch->SearchResults[Index];
			FString ParcelGameValue;
			SearchResult.Session.SessionSettings.Get(
				ParcelSessionSettings::ParcelGameKey,
				ParcelGameValue);
			UE_LOG(
				LogParcelSession,
				Log,
				TEXT("Parcel session result: Index=%d OwnerName=\"%s\" SessionId=%s NumOpenPublicConnections=%d PARCEL_GAME=\"%s\" bUsesPresence=%d bUseLobbiesIfAvailable=%d."),
				Index,
				*SearchResult.Session.OwningUserName,
				*SearchResult.GetSessionIdStr(),
				SearchResult.Session.NumOpenPublicConnections,
				*ParcelGameValue,
				SearchResult.Session.SessionSettings.bUsesPresence,
				SearchResult.Session.SessionSettings.bUseLobbiesIfAvailable);
		}

		if (FilteredResultCount == 0)
		{
			ReportSessionStatus(TEXT("No Parcel_Knight sessions found."), false);
		}
		else
		{
			ReportSessionStatus(
				FString::Printf(TEXT("Found %d Parcel_Knight session(s)."), FilteredResultCount),
				false);
		}
	}
	else
	{
		ReportSessionStatus(
			TEXT("FindSessions failed: Steam session search did not complete successfully."),
			true);
	}

	OnSessionFindComplete.Broadcast(bCompletedSuccessfully);
}

void USessionSubsystem::OnJoinSessionComplete(
	FName SessionName,
	EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	const FName OnlineSubsystemName = OSS ? OSS->GetSubsystemName() : NAME_None;
	const IOnlineSessionPtr Sessions = OSS ? OSS->GetSessionInterface() : nullptr;

	UE_LOG(
		LogParcelSession,
		Log,
		TEXT("JoinSession complete: Session=%s Result=%s(%d) OnlineSubsystem=%s SessionInterfaceValid=%d CurrentOperation=%d."),
		*SessionName.ToString(),
		ParcelSessionMaps::JoinResultToString(Result),
		static_cast<int32>(Result),
		*OnlineSubsystemName.ToString(),
		Sessions.IsValid(),
		static_cast<uint8>(CurrentOperation));

	if (SessionName != NAME_GameSession || CurrentOperation != ESessionOperation::Joining)
	{
		UE_LOG(
			LogParcelSession,
			Warning,
			TEXT("JoinSession completion ignored: ExpectedSession=%s IsJoining=%d ClientTravelCalled=0."),
			*FName(NAME_GameSession).ToString(),
			CurrentOperation == ESessionOperation::Joining);
		return;
	}

	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Joining);
	const bool bTimedOut = bOperationFailureReported;
	ClearOperationState();
	if (bLeaveInProgress)
	{
		UE_LOG(LogParcelSession, Log, TEXT("JoinSession completion converted to pending Leave. ClientTravelCalled=0."));
		BeginDestroySession(EDestroyIntent::Leave);
		return;
	}

	if (bTimedOut)
	{
		UE_LOG(LogParcelSession, Warning, TEXT("Ignoring late JoinSession completion after timeout. ClientTravelCalled=0."));
		CleanupNamedSession();
		return;
	}

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(
			LogParcelSession,
			Error,
			TEXT("JoinSession completion failed: Result=%s(%d) OnlineSubsystem=%s ClientTravelCalled=0."),
			ParcelSessionMaps::JoinResultToString(Result),
			static_cast<int32>(Result),
			*OnlineSubsystemName.ToString());
		FString FailureReason;
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::SessionIsFull:
			FailureReason = TEXT("the session is full");
			break;
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			FailureReason = TEXT("the invited session no longer exists");
			break;
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
			FailureReason = TEXT("Steam could not retrieve the host address");
			break;
		case EOnJoinSessionCompleteResult::AlreadyInSession:
			FailureReason = TEXT("the local user is already in GameSession");
			break;
		default:
			FailureReason = Sessions.IsValid()
				? TEXT("the online subsystem returned an unknown error")
				: TEXT("the Steam session interface became unavailable");
			break;
		}
		ReportJoinFailure(FailureReason);
		OnSessionJoinComplete.Broadcast(false);
		CleanupNamedSession();
		return;
	}

	if (!Sessions.IsValid())
	{
		UE_LOG(
			LogParcelSession,
			Error,
			TEXT("JoinSession succeeded, but the session interface is invalid. OnlineSubsystem=%s ClientTravelCalled=0."),
			*OnlineSubsystemName.ToString());
		ReportJoinFailure(FString::Printf(
			TEXT("the %s online subsystem has no valid session interface"),
			*OnlineSubsystemName.ToString()));
		OnSessionJoinComplete.Broadcast(false);
		CleanupNamedSession();
		return;
	}

	FString ConnectString;
	const bool bResolvedConnectString = Sessions->GetResolvedConnectString(
		NAME_GameSession,
		ConnectString);
	UE_LOG(
		LogParcelSession,
		Log,
		TEXT("GetResolvedConnectString: Session=%s Success=%d ConnectString=\"%s\"."),
		*FName(NAME_GameSession).ToString(),
		bResolvedConnectString,
		*ConnectString);

	if (!bResolvedConnectString)
	{
		UE_LOG(LogParcelSession, Error, TEXT("GetResolvedConnectString returned false. ClientTravelCalled=0."));
		ReportJoinFailure(TEXT("GetResolvedConnectString returned false for GameSession"));
		OnSessionJoinComplete.Broadcast(false);
		CleanupNamedSession();
		return;
	}

	if (ConnectString.IsEmpty())
	{
		UE_LOG(LogParcelSession, Error, TEXT("GetResolvedConnectString returned an empty ConnectString. ClientTravelCalled=0."));
		ReportJoinFailure(TEXT("GetResolvedConnectString returned an empty address for GameSession"));
		OnSessionJoinComplete.Broadcast(false);
		CleanupNamedSession();
		return;
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = GetGameInstance();
	ULocalPlayer* LocalPlayer = GameInstance ? GameInstance->GetLocalPlayerByIndex(0) : nullptr;
	APlayerController* PlayerController = GameInstance
		? GameInstance->GetFirstLocalPlayerController(World)
		: nullptr;
	const bool bIsLocalController = PlayerController && PlayerController->IsLocalController();
	UE_LOG(
		LogParcelSession,
		Log,
		TEXT("Join travel objects: WorldValid=%d GameInstanceValid=%d LocalPlayerValid=%d PlayerControllerValid=%d IsLocalController=%d."),
		World != nullptr,
		GameInstance != nullptr,
		LocalPlayer != nullptr,
		PlayerController != nullptr,
		bIsLocalController);

	if (!World)
	{
		UE_LOG(LogParcelSession, Error, TEXT("JoinSession succeeded, but the GameInstance has no active World. ClientTravelCalled=0."));
		ReportJoinFailure(TEXT("there is no active World for ClientTravel"));
		OnSessionJoinComplete.Broadcast(false);
		CleanupNamedSession();
		return;
	}

	if (!LocalPlayer)
	{
		UE_LOG(LogParcelSession, Error, TEXT("JoinSession succeeded, but local player index 0 is unavailable. ClientTravelCalled=0."));
		ReportJoinFailure(TEXT("local player index 0 is unavailable"));
		OnSessionJoinComplete.Broadcast(false);
		CleanupNamedSession();
		return;
	}

	if (!PlayerController)
	{
		UE_LOG(LogParcelSession, Error, TEXT("JoinSession succeeded, but GetFirstLocalPlayerController returned null. ClientTravelCalled=0."));
		ReportJoinFailure(TEXT("the local PlayerController is unavailable"));
		OnSessionJoinComplete.Broadcast(false);
		CleanupNamedSession();
		return;
	}

	if (!bIsLocalController)
	{
		UE_LOG(LogParcelSession, Error, TEXT("JoinSession succeeded, but the selected PlayerController is not local. ClientTravelCalled=0."));
		ReportJoinFailure(TEXT("the selected PlayerController is not local"));
		OnSessionJoinComplete.Broadcast(false);
		CleanupNamedSession();
		return;
	}

	ReportSessionStatus(TEXT("JoinSession succeeded. Traveling to the host."), false);
	bClientTravelInProgress = true;
	UE_LOG(
		LogParcelSession,
		Log,
		TEXT("Calling ClientTravel: ConnectString=\"%s\" TravelType=TRAVEL_Absolute ClientTravelCalled=1."),
		*ConnectString);
	PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
	UE_LOG(LogParcelSession, Log, TEXT("ClientTravel call returned. ClientTravelCalled=1."));
	OnSessionJoinComplete.Broadcast(true);
}

void USessionSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	bHostTravelInProgress = false;
	bClientTravelInProgress = false;
	if (IsFrontendWorld(LoadedWorld))
	{
		bLeaveInProgress = false;
	}
}

void USessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionName != NAME_GameSession || CurrentOperation != ESessionOperation::Destroying)
	{
		return;
	}

	const IOnlineSessionPtr Sessions = GetSessionInterface();
	ClearDelegateHandleForOperation(Sessions, ESessionOperation::Destroying);
	const EDestroyIntent CompletedIntent = DestroyIntent;
	const bool bTimedOut = bOperationFailureReported;
	DestroyIntent = EDestroyIntent::None;
	ClearOperationState();

	if (bTimedOut)
	{
		UE_LOG(LogParcelSession, Warning, TEXT("Ignoring late DestroySession completion after timeout."));
		PendingNumConnections = 0;
		PendingJoinResult = FOnlineSessionSearchResult();
		bHasPendingJoinResult = false;
		if (CompletedIntent == EDestroyIntent::Leave)
		{
			TravelToFrontend(true);
		}
		return;
	}

	CompleteDestroyIntent(CompletedIntent, bWasSuccessful);
}

void USessionSubsystem::HandleNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	if (!World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	UE_LOG(
		LogParcelSession,
		Error,
		TEXT("Network failure %d: %s"),
		static_cast<uint8>(FailureType),
		*ErrorString);
	if (bClientTravelInProgress)
	{
		ReportJoinFailure(FString::Printf(
			TEXT("ClientTravel connection failed with network error %d: %s"),
			static_cast<uint8>(FailureType),
			*ErrorString));
	}
	if (bLeaveInProgress)
	{
		return;
	}

	const bool bWasClient = World->GetNetMode() == NM_Client ||
		(NetDriver && NetDriver->ServerConnection != nullptr);
	if (bWasClient)
	{
		LeaveSession();
	}
}

void USessionSubsystem::HandleTravelFailure(
	UWorld* World,
	ETravelFailure::Type FailureType,
	const FString& ErrorString)
{
	if (!World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	UE_LOG(
		LogParcelSession,
		Error,
		TEXT("Travel failure %d: %s"),
		static_cast<uint8>(FailureType),
		*ErrorString);
	if (bClientTravelInProgress)
	{
		ReportJoinFailure(FString::Printf(
			TEXT("ClientTravel failed with travel error %d: %s"),
			static_cast<uint8>(FailureType),
			*ErrorString));
	}
	if (bLeaveInProgress)
	{
		bHostTravelInProgress = false;
		bClientTravelInProgress = false;
		// If Leave is waiting for a non-cancellable Steam callback, preserve the
		// leave intent. The callback will retry cleanup/travel instead of allowing
		// a late Create/Start/Join result to resume normal play.
		if (CurrentOperation == ESessionOperation::None)
		{
			bLeaveInProgress = false;
		}
		return;
	}

	bHostTravelInProgress = false;
	bClientTravelInProgress = false;

	const IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession))
	{
		LeaveSession();
	}
}
