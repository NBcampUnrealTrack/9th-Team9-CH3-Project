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

	// [Client] 사망 시 관전 시작 — HandleDeath에서 Client RPC로 호출됨
	void StartSpectating();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* P) override;
	virtual bool InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad) override;

	// [Editor] HUD 위젯 클래스 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ParcelUI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	// HUD 위젯 인스턴스
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ParcelUI")
	TObjectPtr<UUserWidget> HUDWidgetInstance;

private:
	void SpectateNext();
	void SpectatePrev();

	// 현재 살아있는 다른 플레이어 폰 목록 반환
	TArray<APawn*> GetAlivePawns() const;

	bool bIsSpectating = false;
	int32 SpectatorTargetIndex = 0;
};
