// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/SessionSubsystem.h"
#include "Core/ParcelGameInstance.h"
#include "AdvancedFriendsLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"

void USessionSubsystem::ClearOperationState()
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(OperationTimeoutHandle);
	bIsOperationInProgress = false;
	CurrentOperation = ESessionOperation::None;
}

void USessionSubsystem::CancelCurrentOperation()
{
	// 등록된 OSS 델리게이트 먼저 해제 — 취소 후 구 콜백이 재발동되지 않도록
	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr Sessions = OSS->GetSessionInterface())
		{
			switch (CurrentOperation)
			{
			case ESessionOperation::Creating: Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle); break;
			case ESessionOperation::Finding:  Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);  break;
			case ESessionOperation::Joining:  Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);  
           JoinSessionHandle.Reset();
          break;
			default: break;
			}
		}
	}

	const ESessionOperation Op = CurrentOperation;
	ClearOperationState();

	switch (Op)
	{
	case ESessionOperation::Creating: OnSessionCreateComplete.Broadcast(false); break;
	case ESessionOperation::Finding:  OnSessionFindComplete.Broadcast(false);   break;
	case ESessionOperation::Joining:  OnSessionJoinComplete.Broadcast(false);   break;
	default: break;
	}
}

void USessionSubsystem::OnOperationTimeout()
{
	if (!bIsOperationInProgress) return;
	CancelCurrentOperation();
}

void USessionSubsystem::CreateSession(int32 NumPublicConnections)
{
	// 다른 작업 중이면 취소; 이미 Creating 중인 경우(Destroy→Create 내부 재진입)는 그대로 유지
	if (bIsOperationInProgress && CurrentOperation != ESessionOperation::Creating)
		CancelCurrentOperation();

	// Creating 상태로 진입 (재진입 시 타이머 재시작)
	if (UWorld* World = GetWorld())
		World->GetTimerManager().SetTimer(OperationTimeoutHandle, this, &USessionSubsystem::OnOperationTimeout, OperationTimeoutSeconds, false);
	bIsOperationInProgress = true;
	CurrentOperation = ESessionOperation::Creating;

	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) { CancelCurrentOperation(); return; }

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) { CancelCurrentOperation(); return; }

	// 동일 이름 세션이 이미 있으면 파괴 후 자동 재생성 (OnDestroySessionComplete에서 이어받음)
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		bPendingCreate = true;
		PendingNumConnections = NumPublicConnections;
		DestroySession();
		return;
	}

	FOnlineSessionSettings SessionSettings;
	// Null OSS이면 LAN 모드, EOS·Steam이면 온라인 모드 — ini 변경만으로 전환 가능
	SessionSettings.bIsLANMatch = OSS->GetSubsystemName() == "NULL";
	SessionSettings.NumPublicConnections = NumPublicConnections;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowInvites = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = OSS->GetSubsystemName() == FName(TEXT("STEAM"));
	SessionSettings.bAllowJoinInProgress = true;
	// AppId 480(SpaceWar) 공용 테스트 환경에서 다른 팀 세션과 구분하기 위한 식별 키

	CreateSessionHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnCreateSessionComplete)
	);
	Sessions->CreateSession(0, NAME_GameSession, SessionSettings);
}

void USessionSubsystem::FindSessions()
{
	if (bIsOperationInProgress)
		CancelCurrentOperation();

	if (UWorld* World = GetWorld())
		World->GetTimerManager().SetTimer(OperationTimeoutHandle, this, &USessionSubsystem::OnOperationTimeout, OperationTimeoutSeconds, false);
	bIsOperationInProgress = true;
	CurrentOperation = ESessionOperation::Finding;

	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) { CancelCurrentOperation(); return; }

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) { CancelCurrentOperation(); return; }

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = 5000;
	SessionSearch->bIsLanQuery = OSS->GetSubsystemName() == "NULL";
	SessionSearch->TimeoutInSeconds = 10.0f;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	

	FindSessionsHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnFindSessionsComplete)
	);
	Sessions->FindSessions(0, SessionSearch.ToSharedRef());
}

void USessionSubsystem::JoinSession(int32 SessionIndex)
{
    if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(SessionIndex)) 
      return;
    StartJoinSession(SessionSearch->SearchResults[SessionIndex]);
}


bool USessionSubsystem::JoinSessionResult(const FOnlineSessionSearchResult& SessionResult)
{
    return StartJoinSession(SessionResult);
}

bool USessionSubsystem::StartJoinSession(const FOnlineSessionSearchResult& SessionResult)
{
    if (!SessionResult.IsValid()) { OnSessionJoinComplete.Broadcast(false); return false; }

    if (bIsOperationInProgress)
        CancelCurrentOperation();

    if (UWorld* World = GetWorld())
        World->GetTimerManager().SetTimer(OperationTimeoutHandle, this, &USessionSubsystem::OnOperationTimeout, OperationTimeoutSeconds, false);
    bIsOperationInProgress = true;
    CurrentOperation = ESessionOperation::Joining;

    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (!OSS) { CancelCurrentOperation(); return false; }

    IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
    if (!Sessions.IsValid()) { CancelCurrentOperation(); return false; }

    if (JoinSessionHandle.IsValid())
    {
        Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
        JoinSessionHandle.Reset();
    }

    JoinSessionHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
        FOnJoinSessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnJoinSessionComplete)
    );

    if (!Sessions->JoinSession(0, NAME_GameSession, SessionResult))
    {
        Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
        JoinSessionHandle.Reset();
        CancelCurrentOperation();
        return false;
    }

    return true;
}

void USessionSubsystem::DestroySession()
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;

	DestroySessionHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnDestroySessionComplete)
	);
	
	Sessions->DestroySession(NAME_GameSession);
}


void USessionSubsystem::StartGame(const FString& MapPath)
{
	GetWorld()->ServerTravel(MapPath + "?listen");
}

int32 USessionSubsystem::GetSearchResultCount() const
{
	if (SessionSearch.IsValid())
		return SessionSearch->SearchResults.Num();
	return 0;
}

FString USessionSubsystem::GetSessionOwnerName(int32 Index) const
{
	if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(Index))
		return TEXT("");
	// OwningUserName: OSS가 기록한 세션 호스트의 플레이어 이름, Steam사용?
	return SessionSearch->SearchResults[Index].Session.OwningUserName;
}

int32 USessionSubsystem::GetSessionPlayerCount(int32 Index) const
{
	if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(Index))
		return 0;
	const FOnlineSession& Session = SessionSearch->SearchResults[Index].Session;
	// 최대 인원 - 남은 빈 슬롯 = 현재 접속 인원
	return Session.SessionSettings.NumPublicConnections - Session.NumOpenPublicConnections;
}

bool USessionSubsystem::CanInviteToCurrentSession() const
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS || !GetWorld() || GetWorld()->GetNetMode() == NM_Client)
	{
		return false;
	}

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid())
	{
		return false;
	}

	FNamedOnlineSession* NamedSession = Sessions->GetNamedSession(NAME_GameSession);
	if (!NamedSession || !NamedSession->SessionSettings.bAllowInvites)
	{
		return false;
	}

	const EOnlineSessionState::Type SessionState = Sessions->GetSessionState(NAME_GameSession);
	if (SessionState == EOnlineSessionState::NoSession || SessionState == EOnlineSessionState::Destroying)
	{
		return false;
	}

	IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
	const TSharedPtr<const FUniqueNetId> LocalUserId = Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
	if (LocalUserId.IsValid() && NamedSession->OwningUserId.IsValid())
	{
		return *LocalUserId == *NamedSession->OwningUserId;
	}

	// 일부 로컬/개발 OSS는 소유자 ID를 채우지 않으므로 listen host 여부를 안전한 fallback으로 사용합니다.
	return GetWorld()->GetNetMode() == NM_ListenServer;
}

bool USessionSubsystem::SendSessionInviteToFriend(
	APlayerController* PlayerController,
	const FBPUniqueNetId& FriendUniqueNetId) const
{
	if (!PlayerController || !PlayerController->IsLocalController() ||
		!CanInviteToCurrentSession() || !FriendUniqueNetId.IsValid())
	{
		return false;
	}

	EBlueprintResultSwitch Result = EBlueprintResultSwitch::OnFailure;
	UAdvancedFriendsLibrary::SendSessionInviteToFriend(PlayerController, FriendUniqueNetId, Result);
	return Result == EBlueprintResultSwitch::OnSuccess;
}

//---------------세션 컴플리트----------------------------------------------------------

void USessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;

	Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
	ClearOperationState();
	OnSessionCreateComplete.Broadcast(bWasSuccessful);
	if (bWasSuccessful && GetWorld()->GetNetMode() != NM_Client)
	{
		Sessions->StartSession(NAME_GameSession);
		if (UParcelGameInstance* GI = Cast<UParcelGameInstance>(GetGameInstance()))
			GetWorld()->ServerTravel(GI->GetPendingMapPath() + "?listen");
	}
}

void USessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;

	Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
	ClearOperationState();
	OnSessionFindComplete.Broadcast(bWasSuccessful);
}

void USessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;

	Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
	JoinSessionHandle.Reset();
	ClearOperationState();
  
	OnSessionJoinComplete.Broadcast(Result == EOnJoinSessionCompleteResult::Success);
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		FString TravelURL;
		// OSS 내부 주소를 클라이언트가 접속 가능한 IP:Port 형태로 변환
		if (Sessions->GetResolvedConnectString(NAME_GameSession, TravelURL))
		{
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (PC) PC->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
		}
	}
}

void USessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;
	
	Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);

	// CreateSession 중 기존 세션 제거였으면 Broadcast 없이 바로 재생성
	if (bPendingCreate)
	{
		bPendingCreate = false;
		CreateSession(PendingNumConnections);
		return;
	}

	OnSessionDestroyComplete.Broadcast(bWasSuccessful);

	
}
