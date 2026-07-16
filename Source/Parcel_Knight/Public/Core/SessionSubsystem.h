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
UCLASS(BlueprintType)
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
	
	
	UFUNCTION(BlueprintCallable)
	void CreateSession(int32 NumPublicConnections);

	UFUNCTION(BlueprintCallable)
	void FindSessions();

	// Blueprint에서 FOnlineSessionSearchResult를 직접 쓸 수 없으므로 인덱스로 참가
	UFUNCTION(BlueprintCallable)
	void JoinSession(int32 SessionIndex);

	UFUNCTION(BlueprintCallable)
	void DestroySession();

	// TODO: 로비맵 완성 후 OnCreateSessionComplete의 이동 대상을 로비맵으로 교체
	UFUNCTION(BlueprintCallable)
	void StartGame(const FString& MapPath);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetSearchResultCount() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetSessionOwnerName(int32 Index) const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetSessionPlayerCount(int32 Index) const;
	
private:
	enum class ESessionOperation : uint8 { None, Creating, Finding, Joining };

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	// 진행 중인 작업을 취소하고 실패 브로드캐스트
	void CancelCurrentOperation();

	// 타임아웃 발동 시 호출
	void OnOperationTimeout();

	// 타임아웃 타이머를 취소하고 진행 상태를 초기화
	void ClearOperationState();

	// 세션 파괴 완료 후 자동 재생성을 위한 플래그
	bool bPendingCreate = false;
	int32 PendingNumConnections = 0;

	bool bIsOperationInProgress = false;
	ESessionOperation CurrentOperation = ESessionOperation::None;

	// 응답 없을 때 강제 리셋까지 대기 시간 (초)
	static constexpr float OperationTimeoutSeconds = 30.f;
	FTimerHandle OperationTimeoutHandle;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FDelegateHandle CreateSessionHandle;
	FDelegateHandle FindSessionsHandle;
	FDelegateHandle JoinSessionHandle;
	FDelegateHandle DestroySessionHandle;
};
