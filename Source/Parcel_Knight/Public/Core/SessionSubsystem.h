#pragma once

#include "CoreMinimal.h"
#include "BlueprintDataDefinitions.h"
#include "Engine/EngineBaseTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SessionSubsystem.generated.h"

class APlayerController;
class UNetDriver;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionCreateComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionSearchStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionFindComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionJoinComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionDestroyComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnSessionStatusMessage,
	const FText&,
	Message,
	bool,
	bIsError);

/**
 * Owns the asynchronous online-session flow for the lifetime of the GameInstance.
 * Widgets only call this public API and receive the existing Blueprint delegates.
 */
UCLASS(BlueprintType)
class PARCEL_KNIGHT_API USessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable)
	FOnSessionCreateComplete OnSessionCreateComplete;

	/** Emitted after stale search results are cleared and before a new request starts. */
	UPROPERTY(BlueprintAssignable)
	FOnSessionSearchStarted OnSessionSearchStarted;

	UPROPERTY(BlueprintAssignable)
	FOnSessionFindComplete OnSessionFindComplete;

	UPROPERTY(BlueprintAssignable)
	FOnSessionJoinComplete OnSessionJoinComplete;

	UPROPERTY(BlueprintAssignable)
	FOnSessionDestroyComplete OnSessionDestroyComplete;

	/** Human-readable invite/join state for Blueprint UI. */
	UPROPERTY(BlueprintAssignable)
	FOnSessionStatusMessage OnSessionStatusMessage;

	UFUNCTION(BlueprintCallable)
	void CreateSession(int32 NumPublicConnections);

	UFUNCTION(BlueprintCallable)
	void FindSessions();

	UFUNCTION(BlueprintCallable)
	void JoinSession(int32 SessionIndex);

	// Used by the native Steam invite-accepted callback.
	bool JoinSessionResult(const FOnlineSessionSearchResult& SessionResult);

	// Reports Steam invite acceptance through the same UI/log path as JoinSession.
	void NotifySessionInviteAccepted(bool bWasSuccessful);

	UFUNCTION(BlueprintCallable)
	void DestroySession();

	/** Destroys the local named session and returns to the existing front-end map. */
	UFUNCTION(BlueprintCallable)
	void LeaveSession();

	UFUNCTION(BlueprintCallable)
	void StartGame(const FString& MapPath);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetSearchResultCount() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetSessionOwnerName(int32 Index) const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetSessionPlayerCount(int32 Index) const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool CanInviteToCurrentSession() const;

	bool SendSessionInviteToFriend(
		APlayerController* PlayerController,
		const FBPUniqueNetId& FriendUniqueNetId) const;

private:
	enum class ESessionOperation : uint8
	{
		None,
		Creating,
		Starting,
		Finding,
		Joining,
		Destroying
	};

	enum class EDestroyIntent : uint8
	{
		None,
		UserRequested,
		Recreate,
		JoinPending,
		Leave,
		Cleanup
	};

	IOnlineSessionPtr GetSessionInterface() const;
	bool GetSteamSessionInterface(IOnlineSessionPtr& OutSessions, const TCHAR* Context) const;

	void BeginOperation(ESessionOperation NewOperation);
	void ClearOperationState();
	void ClearDelegateHandleForOperation(const IOnlineSessionPtr& Sessions, ESessionOperation Operation);
	void ClearAllDelegateHandles(const IOnlineSessionPtr& Sessions);
	void ReportOperationFailure(ESessionOperation Operation);
	void ReportSessionStatus(const FString& Message, bool bIsError);
	void ReportJoinFailure(const FString& Reason);
	void OnOperationTimeout();

	bool BeginCreateSession(int32 NumPublicConnections);
	bool StartJoinSession(const FOnlineSessionSearchResult& SessionResult);
	bool BeginJoinSession(const FOnlineSessionSearchResult& SessionResult);
	bool BeginDestroySession(EDestroyIntent Intent);
	void CompleteDestroyIntent(EDestroyIntent Intent, bool bWasSuccessful);
	void CleanupNamedSession();
	void TravelToFrontend(bool bFinishLeaveFlow = true);
	bool IsFrontendWorld(const UWorld* World) const;
	bool IsSessionTransitionLocked() const;

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandlePostLoadMap(UWorld* LoadedWorld);

	void HandleNetworkFailure(
		UWorld* World,
		UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType,
		const FString& ErrorString);
	void HandleTravelFailure(
		UWorld* World,
		ETravelFailure::Type FailureType,
		const FString& ErrorString);

	static constexpr float OperationTimeoutSeconds = 30.0f;

	ESessionOperation CurrentOperation = ESessionOperation::None;
	EDestroyIntent DestroyIntent = EDestroyIntent::None;
	bool bOperationFailureReported = false;
	bool bLeaveInProgress = false;
	bool bHostTravelInProgress = false;
	bool bClientTravelInProgress = false;

	int32 PendingNumConnections = 0;
	FOnlineSessionSearchResult PendingJoinResult;
	bool bHasPendingJoinResult = false;

	FTimerHandle OperationTimeoutHandle;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	FDelegateHandle CreateSessionHandle;
	FDelegateHandle StartSessionHandle;
	FDelegateHandle FindSessionsHandle;
	FDelegateHandle JoinSessionHandle;
	FDelegateHandle DestroySessionHandle;
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
	FDelegateHandle PostLoadMapHandle;
};
