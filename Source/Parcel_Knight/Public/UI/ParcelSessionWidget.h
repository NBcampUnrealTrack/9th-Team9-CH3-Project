#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParcelSessionWidget.generated.h"

class USessionSubsystem;

/**
 * 세션 관련 UI의 C++ 베이스 클래스
 * Blueprint에서 버튼 OnClicked → BlueprintCallable 함수 연결만 하면 됨
 * delegate 바인딩·해제는 NativeConstruct/NativeDestruct에서 자동 처리
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelSessionWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	// ── Blueprint 버튼 OnClicked에 연결할 함수들 ──────────────────────────

	UFUNCTION(BlueprintCallable)
	void CreateSession(int32 NumPlayers);

	UFUNCTION(BlueprintCallable)
	void FindSessions();

	UFUNCTION(BlueprintCallable)
	void JoinSession(int32 SessionIndex);

	UFUNCTION(BlueprintCallable)
	void SetMapPath(const FString& MapPath);

	// ── Blueprint에서 구현 — C++이 결과 전달 시 호출됨 ────────────────────

	UFUNCTION(BlueprintImplementableEvent)
	void OnSessionCreated(bool bSuccess);

	UFUNCTION(BlueprintImplementableEvent)
	void OnSessionsFound(bool bSuccess);

	UFUNCTION(BlueprintImplementableEvent)
	void OnSessionJoined(bool bSuccess);

	/** Invite acceptance and JoinSession details suitable for a status text/toast. */
	UFUNCTION(BlueprintImplementableEvent)
	void OnSessionStatusChanged(const FText& Message, bool bIsError);

private:
	UFUNCTION()
	void HandleSessionCreateComplete(bool bWasSuccessful);

	UFUNCTION()
	void HandleSessionFindComplete(bool bWasSuccessful);

	UFUNCTION()
	void HandleSessionJoinComplete(bool bWasSuccessful);

	UFUNCTION()
	void HandleSessionStatusMessage(const FText& Message, bool bIsError);

	USessionSubsystem* GetSessionSubsystem() const;
};
