// Fill out your copyright notice in the Description page of Project Settings.

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

	void StartRound();
	void EndRound();

protected:
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

private:
	UPROPERTY()
	TObjectPtr<UDeliveryRuleComponent> DeliveryRuleComp;
	//스테이지데이터를 받아서 설정
	UPROPERTY(EditDefaultsOnly, Category = "Stage")
	TObjectPtr<UStageData> CurrentStageData;
	
	
};
