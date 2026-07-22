#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ParcelGameMode.generated.h"

class AParcelPlayerController;
class AParcelPlayerState;
class AParcelCharacter;
class UDeliveryRuleComponent;
class URespawnComponent;
class AParcelGameState;
class UStageData;
class UParcelCheatManager;
class UDataTable;
class USessionSubsystem;

/**
 * 게임 룰을 관리하는 GameMode — 함수의 실행만 담당
 * 실제 로직은 컴포넌트에 위임한다.
 *   배달·등급·보상 → UDeliveryRuleComponent
 *   사망·부활      → URespawnComponent
 *
 * 담당자: 한수현
 */
UCLASS()
class PARCEL_KNIGHT_API AParcelGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AParcelGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual bool AllowCheats(APlayerController* P) override { return true; }

	void StartRound(UStageData* InStageData);
	void EndRound();

	// 배달 성공/실패 진입점 — 다른 팀원 코드에서 이 함수만 호출
	void OnDeliveryCompleted(APlayerController* Deliverer, const FString& BoxName, int32 ScoreAmount);
	void OnDeliveryFailed(APlayerController* Deliverer, const FString& BoxName, int32 ScoreAmount = 0);

	/** Server-authoritative lobby map selection. Rejects non-owners and invalid catalog indices. */
	bool RequestLobbyMapSelection(APlayerController* RequestingController, int32 NewMapIndex);

	/** Server-authoritative lobby start. Resolves the selected map from the trusted catalog. */
	bool RequestStartLobbyGame(APlayerController* RequestingController);

	// [All] 부활 컴포넌트 반환 — PlayerState::HandleDeath에서 사용
	URespawnComponent* GetRespawnComponent() const;

protected:
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

private:
	friend class USessionSubsystem;

	bool IsAuthorizedLobbyHost(const APlayerController* RequestingController) const;
	bool IsSelectedLobbyMapPath(const FString& CandidateMapPath) const;
	bool ResolveLobbyMapPath(int32 MapIndex, FString& OutMapPath) const;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UDeliveryRuleComponent> DeliveryRuleComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<URespawnComponent> RespawnComp;

	// 현재 스테이지 데이터 — 에디터(BP_ParcelGameMode)에서 지정하거나 레벨 BP에서 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStageData> CurrentStageData;

	/** Existing DT_LobbyMapData, promoted to the server-owned allowlist for lobby travel. */
	UPROPERTY(EditDefaultsOnly, Category = "Lobby|Map", meta=(AllowPrivateAccess="true"))
	TSoftObjectPtr<UDataTable> LobbyMapCatalog;
};
