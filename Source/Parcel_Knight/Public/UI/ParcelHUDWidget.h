#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "ParcelHUDWidget.generated.h"

class AParcelGameState;
class ADeliveryBox;
class UButton;
class UParcelFriendListWidget;
/**
 *  UParcelWidget
 *  인게임 HUD 요소의 이벤트 바인딩하고 관리하는 베이스 위젯
 *  담당자 : JYW
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	// 위젯의 BeginPlay 함수
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	UFUNCTION(BlueprintCallable, Category = "ParcelUI|Friend")
	void OpenFriendList();

	UFUNCTION(BlueprintCallable, Category = "ParcelUI|Friend")
	void ToggleFriendList();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> FriendButton;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ParcelUI|Friend")
	TSubclassOf<UParcelFriendListWidget> FriendListWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "ParcelUI|Friend")
	TObjectPtr<UParcelFriendListWidget> FriendListWidgetInstance;
	
	// 로컬 플레이어와 데이터를 바인딩
	void TryBindUIEvents();
	
	// WBP에서 사용할 수 있도록 열어두는 이벤트 (델리게이트 -> 브로드캐스트. K2는 Kismet2 약자입니다)
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void K2_OnHPChanged(float CurrentHP, float MaxHP);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void K2_OnTeamScoreChanged(const FText& DisplayText);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "ParcelUI")
	void K2_OnRemainingTimeChanged(const FText& DisplayText, float RawRemainingTime);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "ParcelUI")
	void K2_OnCrosshairStateChanged(bool bCanInteract, const FText& InteractionPrompt);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "ParcelUI")
	void K2_OnCarriedBoxInfoChanged(bool bIsCarrying, const FText& BoxTypeName, const FText& DestinationText, FGameplayTag BoxTypeTag);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "ParcelUI")
	void K2_OnComboChanged(int32 NewComboCount, const FText& DisplayText);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "ParcelUI")
	void K2_OnCharacterStateChanged(const FGameplayTagContainer& ActiveTags);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "ParcelUI")
	void K2_OnThrowChargeChanged(bool bIsCharging, float ChargeRatio);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "ParcelUI")
	void K2_OnStaminaChanged(float CurrentStamina, float MaxStamina);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "ParcelUI|Events")
	void K2_OnDeliveryLogAdded(const FString& PlayerName, const FString& BoxName, bool bSuccess);
	
private:
	
	UFUNCTION()
	void HandleOnTeamScoreChanged(int32 NewTeamScore);
	
	UFUNCTION()
	void HandleOnComboChanged(int32 NewComboCount);
	
	// [Timestamp] 핸들러(종료 시각)
	UFUNCTION()
	void HandleOnExpirationTimeChanged(float NewExpirationTime);
	
	UFUNCTION() 
	void HandleOnInteractionFocusChanged(AActor* NewFocusedActor);
	
	UFUNCTION() 
	void HandleOnCarriedBoxChanged(ADeliveryBox* NewCarriedBox);
	
	UFUNCTION()
	void HandleOnCharacterStateChanged(const FGameplayTagContainer& ActiveTags);
	
	UFUNCTION()
	void HandleOnThrowChargeChanged(bool bIsCharging, float ChargeRatio);
	
	UFUNCTION()
	void HandleOnStaminaChanged(float CurrentStamina, float MaxStamina);
	
	UFUNCTION()
	void HandleOnDeliveryLogReceived(const FString& PlayerName, const FString& BoxName, bool bSuccess);
	
	// [Timestamp] (남은 시간)
	void UpdateLocalTimer();
	
	// 안전한 접근을 위해 캐싱
	UPROPERTY()
	TWeakObjectPtr<AParcelGameState> CachedGameState;
	
	// 안전장치용 타이머 핸들
	FTimerHandle RetryBindTimerHandle;
	
	// [Timestamp] UI 자체 로컬 타이머 핸들 및 종료 시간 저장 변수
	FTimerHandle UILocalTimerHandle;
	float CachedExpirationTime = 0.0f;
};
