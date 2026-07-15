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

public:
	AParcelPlayerController();

protected:
	virtual void BeginPlay() override;
	
	virtual void SetupInputComponent() override;
	
	// [Editor] HUD 위젯 클래스 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ParcelUI")
	TSubclassOf<UUserWidget> HUDWidgetClass;
	
	// HUD 위젯 인스턴스
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ParcelUI")
	TObjectPtr<UUserWidget> HUDWidgetInstance;
};
