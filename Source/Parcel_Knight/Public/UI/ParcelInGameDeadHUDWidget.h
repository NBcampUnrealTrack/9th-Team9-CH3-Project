#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "ParcelInGameDeadHUDWidget.generated.h"

class UTextBlock;
class USoundBase;

/**
 * 
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelInGameDeadHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void StartDeathCountdown(int32 TotalSeconds = 5);

protected:
	virtual void NativeConstruct() override;
	
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "ParcelUI|Dead")
	void K2_OnCountdownChanged(int32 NewCount);
	
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "ParcelUI|Dead")
	TObjectPtr<UTextBlock> Txt_CountdownNumber;
	
	UPROPERTY(EditDefaultsOnly, Category = "ParcelUI|Dead|Sound")
	TObjectPtr<USoundBase> HeartbeatSound;

private:
	// 타이머 내부 함수
	void AdvanceCountdown();
	
	void PlayHeartbeatSound();

	FTimerHandle CountdownTimerHandle;
	int32 CurrentCount = 5;
};