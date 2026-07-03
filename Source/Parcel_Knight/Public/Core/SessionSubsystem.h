// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionCreateComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionFindComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionJoinComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionDestroyComplete, bool, bWasSuccessful);

/**
 * 리슨 서버 세션 생성·참가·종료를 담당하는 서브시스템
 * GameInstance에 귀속되며 씬 전환 후에도 세션 상태를 유지한다.
 * OnlineSubsystem 백엔드(Null / EOS / Steam)를 추상화하여
 * ini 설정 변경만으로 교체 가능하다.
 *
 * 담당자: 한수현
 */
UCLASS()
class PARCEL_KNIGHT_API USessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	

	
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnSessionCreateComplete OnSessionCreateComplete;
	
	UPROPERTY(BlueprintAssignable)
	FOnSessionFindComplete OnSessionFindComplete;

	UPROPERTY(BlueprintAssignable)
	FOnSessionJoinComplete OnSessionJoinComplete;

	UPROPERTY(BlueprintAssignable)
	FOnSessionDestroyComplete OnSessionDestroyComplete;
	
	
	void CreateSession(int32 NumPublicConnections);
	void FindSessions();
	void JoinSession(const FOnlineSessionSearchResult& SearchResult);
	void DestroySession();
	// 검색 완료 후 결과를 꺼내는 getter
	TArray<FOnlineSessionSearchResult> GetSearchResults() const;

	
private:
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	
	
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FDelegateHandle CreateSessionHandle;
	FDelegateHandle FindSessionsHandle;
	FDelegateHandle JoinSessionHandle;
	FDelegateHandle DestroySessionHandle;
	

};
