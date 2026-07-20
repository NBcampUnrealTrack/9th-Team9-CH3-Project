// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ParcelPlayerController.generated.h"

class UParcelInGameDeadHUDWidget;
class UParcelInGameESCMenuWidget;
class UParcelLobbyHUDWidget;

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
	
public:
	/** [Client -> Server] 클라이언트가 입력한 채팅을 서버 방장에게 전달하는 Reliable RPC */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendLobbyChatMessage(const FText& ChatText);

	/** [Server -> Client] 서버가 모든 접속자의 로컬 HUD에 채팅방 글을 꽂아주는 브로드캐스트 RPC */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveLobbyChatMessage(const FString& SenderName, const FText& ChatText);

	/** 로비 HUD 위젯 인스턴스 주소를 플레이어 컨트롤러가 안전하게 쥐고 있을 주머니 변수 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ParcelUI")
	TObjectPtr<UParcelLobbyHUDWidget> LobbyHUDWidgetInstance;
	
public:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestChangeLobbyMap(int32 NewMapIndex);
};
