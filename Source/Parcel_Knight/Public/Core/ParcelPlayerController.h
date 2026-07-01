// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ParcelPlayerController.generated.h"

/**
 * 담당자: 김로운
 */
UCLASS()
class PARCEL_KNIGHT_API AParcelPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	virtual void SetupInputComponent() override;
};
