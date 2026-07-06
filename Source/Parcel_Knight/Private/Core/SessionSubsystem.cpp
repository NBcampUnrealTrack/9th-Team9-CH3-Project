// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/SessionSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"

void USessionSubsystem::CreateSession(int32 NumPublicConnections)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;

	// 동일 이름 세션이 이미 있으면 CreateSession이 실패하므로 먼저 제거
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		Sessions->DestroySession(NAME_GameSession);
		return;
	}

	FOnlineSessionSettings SessionSettings;
	// Null OSS이면 LAN 모드, EOS·Steam이면 온라인 모드 — ini 변경만으로 전환 가능
	SessionSettings.bIsLANMatch = OSS->GetSubsystemName() == "NULL";
	SessionSettings.NumPublicConnections = NumPublicConnections;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bAllowJoinInProgress = true;

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

	FindSessionsHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnFindSessionsComplete)
	);
	Sessions->FindSessions(0, SessionSearch.ToSharedRef());
}

void USessionSubsystem::JoinSession(const FOnlineSessionSearchResult& SearchResult)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;

	JoinSessionHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::OnJoinSessionComplete)
	);
	
	Sessions->JoinSession(0, NAME_GameSession, SearchResult);
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


TArray<FOnlineSessionSearchResult> USessionSubsystem::GetSearchResults() const
{
	if (SessionSearch.IsValid())
		return SessionSearch->SearchResults;
	return {};
}

void USessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
	if (!Sessions.IsValid()) return;
	
	Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
	OnSessionCreateComplete.Broadcast(bWasSuccessful);
	if (bWasSuccessful)
		GetWorld()->ServerTravel("/Game/Maps/GameMap?listen"); // ?listen = 리슨 서버 모드, 호스트가 플레이어로도 참여
	
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
	OnSessionDestroyComplete.Broadcast(bWasSuccessful);

	
}
