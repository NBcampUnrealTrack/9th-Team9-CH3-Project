#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ParcelGameMode.generated.h"

class AParcelPlayerController;
class AParcelPlayerState;
class AParcelCharacter;
class UDeliveryRuleComponent;
class AParcelGameState;
class UStageData;

/**
 * 게임 룰을 관리하는 GameMode
 * 실제 로직은 UDeliveryRuleComponent가 담당한다.
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

	void StartRound(UStageData* InStageData);
	void EndRound();

	// 배달 성공/실패 진입점 — 다른 팀원 코드에서 이 함수만 호출
	void OnDeliveryCompleted(APlayerController* Deliverer, int32 ScoreAmount);
	void OnDeliveryFailed(APlayerController* Deliverer);

protected:
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UDeliveryRuleComponent> DeliveryRuleComp;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameRules", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStageData> DefaultStageData;
	
	// ========================= 콘솔 명령어 =========================
	//NOTE: ~(콘솔)에서 해당 명령어 사용시 실제 해당 코드 사용됨
	
	public:
	//배달 성공 및 점수추가
	UFUNCTION(Exec) void DebugDeliverySuccess();
	//배달 실패로 콤보 끊김
	UFUNCTION(Exec) void DebugDeliveryFail();
	//원하는 점수 추가
	UFUNCTION(Exec) void DebugAddScore(int32 Amount);
	//점수 및 콤보 출력
	UFUNCTION(Exec) void DebugPrintScore();
};
