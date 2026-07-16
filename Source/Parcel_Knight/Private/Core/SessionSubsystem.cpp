// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/SessionSubsystem.h"
#include "Core/ParcelGameInstance.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"

void USessionSubsystem::CreateSession(int32 NumPublicConnections)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;

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
	SessionSettings.bUsesPresence = true;
	SessionSettings.bAllowJoinInProgress = true;
	// AppId 480(SpaceWar) 공용 테스트 환경에서 다른 팀 세션과 구분하기 위한 식별 키
	SessionSettings.Set(FName("GAME_ID"), FString("ParcelKnight"), EOnlineDataAdvertisementType::ViaOnlineService);

	CreateSessionHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnCreateSessionComplete)
	);
	Sessions->CreateSession(0, NAME_GameSession, SessionSettings);
}

void USessionSubsystem::FindSessions()
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = 10;
	SessionSearch->bIsLanQuery = OSS->GetSubsystemName() == "NULL";
	SessionSearch->TimeoutInSeconds = 10.0f;
	SessionSearch->QuerySettings.Set(FName("GAME_ID"), FString("ParcelKnight"), EOnlineComparisonOp::Equals);

	FindSessionsHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnFindSessionsComplete)
	);
	Sessions->FindSessions(0, SessionSearch.ToSharedRef());
}

void USessionSubsystem::JoinSession(int32 SessionIndex)
{
	if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(SessionIndex)) return;

	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;

	JoinSessionHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnJoinSessionComplete)
	);
	Sessions->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[SessionIndex]);
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

//---------------세션 컴플리트----------------------------------------------------------

void USessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;
	
	Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
	OnSessionCreateComplete.Broadcast(bWasSuccessful);
	// 이미 다른 세션의 클라이언트로 연결된 상태에서는 ServerTravel이 유효하지 않으므로 제외
	if (bWasSuccessful && GetWorld()->GetNetMode() != NM_Client)
		// GameInstance의 PendingMapPath 읽어서 이동 — UI에서 SetPendingMapPath로 사전 설정
		if (UParcelGameInstance* GI = Cast<UParcelGameInstance>(GetGameInstance()))
			GetWorld()->ServerTravel(GI->GetPendingMapPath() + "?listen");
	
}

void USessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;
	
	Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
	OnSessionFindComplete.Broadcast(bWasSuccessful);
	
	
}

void USessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;
	
	Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
	
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
