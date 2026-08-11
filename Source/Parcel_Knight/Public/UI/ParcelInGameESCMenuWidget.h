#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParcelInGameESCMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UParcelOptionsWidget;
class USoundBase;

/**
 * 인게임 ESC(일시정지) 메뉴의 전체 제어 및 멀티플레이어 바인딩을 담당하는 컨트롤러
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelInGameESCMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	
	UPROPERTY(EditDefaultsOnly, Category = "MainMenu|Sound")
	TObjectPtr<USoundBase> ButtonClickSound;

	// UI Bindings
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Resume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Restart;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Options;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Exit;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_MapName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_PersonalScore;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_PlayerList;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UParcelOptionsWidget> OptionsWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UParcelOptionsWidget> OptionsWidgetInstance = nullptr;

	// Handlers
	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleOptionsClicked();

	UFUNCTION()
	void HandleExitClicked();

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UI", meta = (DisplayName = "OnMenuOpenStarted"))
	void K2_OnMenuOpenStarted();
    
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UI", meta = (DisplayName = "OnMenuCloseStarted"))
	void K2_OnMenuCloseStarted();
    
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "OnTeardownStarted"))
	void K2_OnTeardownStarted();
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetupMenu();
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	void CompleteTeardown();
	
	void RefreshMenuData();
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	bool CloseSubMenuIfOpen();
	
	UFUNCTION()
	void CloseOptionsWidget();

private:
	void PlayButtonClickSound();

	void UpdateMapName();
	void UpdatePersonalScore();
	void UpdatePlayerList();

	// 원인 불명의 중복 생성을 대비한 방어 코드: 뷰포트에 남아있는 UParcelOptionsWidget 인스턴스를
	// 전부 찾아서 OptionsWidgetInstance 하나만 남기고 나머지는 전부 RemoveFromParent 시킨다.
	void CleanupDuplicateOptionsWidgets();
};
