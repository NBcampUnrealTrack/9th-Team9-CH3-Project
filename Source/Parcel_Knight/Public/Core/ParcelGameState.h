// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "ParcelGameState.generated.h"

class UTeamScoreComponent;
class UShopComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDeliveryLogReceivedSignature, const FString&, PlayerName, const FString&, BoxName, bool, bSuccess);

/**
 * 팀 점수 및 남은 시간을 관리하는 GameState
 * 실제 로직은 UTeamScoreComponent가 담당한다.
 *
 * 담당자: 한수현
 */
UCLASS()
class PARCEL_KNIGHT_API AParcelGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	AParcelGameState();
	UTeamScoreComponent* GetTeamScoreComponent() const;
	UShopComponent* GetShopComponent() const;
	
	// [UI]
	UPROPERTY(BlueprintAssignable, Category = "ParcelUI|Events")
	FOnDeliveryLogReceivedSignature OnDeliveryLogReceived;
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyDeliveryLog(const FString& PlayerName, const FString& BoxName, bool bSuccess);

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UTeamScoreComponent> TeamScoreComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UShopComponent> ShopComp;
};

