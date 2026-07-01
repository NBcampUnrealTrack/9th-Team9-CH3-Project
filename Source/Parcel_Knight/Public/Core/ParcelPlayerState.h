// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ParcelPlayerState.generated.h"

class UPlayerStatComponent;

/**
 * 개인 점수, 콤보, 성공·실패 횟수를 관리하는 PlayerState
 * 실제 로직은 UPlayerStatComponent가 담당한다.
 *
 * 담당자: 한수현
 */
UCLASS()
class PARCEL_KNIGHT_API AParcelPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	
	AParcelPlayerState();

	int32 GetPersonalScore() const;
	int32 GetComboCount() const; 
	int32 GetSuccessCount() const;
	
	void AddScore(int32 Amount);
	
	void OnDeliverySuccess();
	
	void OnDeliveryFail();
	
private:
	UPROPERTY()
	UPlayerStatComponent* PlayerStatComp;
};
