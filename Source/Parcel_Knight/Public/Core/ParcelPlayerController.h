// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ParcelPlayerController.generated.h"

class UParcelInGameDeadHUDWidget;
class UParcelInGameESCMenuWidget;

/**
 * 담당자: 김로운
 */
UCLASS()
class PARCEL_KNIGHT_API AParcelPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AParcelPlayerController();
	
	// [Client]
	UFUNCTION(Client, Reliable)
	void Client_NotifyDeath();
	
	// [Client]
	UFUNCTION(Client, Reliable)
	void Client_NotifyRespawn();
	
	UFUNCTION(BlueprintCallable, Category = "ParcelUI")
	void ToggleInGameMenu();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* InPawn) override;
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
	// [Editor] HUD 위젯 클래스 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ParcelUI")
	TSubclassOf<UUserWidget> HUDWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ParcelUI")
	TSubclassOf<UParcelInGameDeadHUDWidget> DeadHUDWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ParcelUI")
	TSubclassOf<UParcelInGameESCMenuWidget> ESCMenuClass;
	
	// HUD 위젯 인스턴스
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ParcelUI")
	TObjectPtr<UUserWidget> HUDWidgetInstance;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ParcelUI")
	TObjectPtr<UParcelInGameDeadHUDWidget> DeadHUDWidgetInstance;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ParcelUI")
	TObjectPtr<UParcelInGameESCMenuWidget> ESCMenuRef;
	
};
