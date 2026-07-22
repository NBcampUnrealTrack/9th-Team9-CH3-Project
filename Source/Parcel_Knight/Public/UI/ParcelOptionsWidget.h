#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParcelOptionsWidget.generated.h"

class USlider;
class UComboBoxString;
class UButton;
class UEditableTextBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOptionsClosedSignature);

/**
 * UParcelOptionsWidget
 * 볼륨, 마우스 감도 및 엔진 그래픽 세팅(해상도, 화면모드)을 조작하고 보관하는 위젯
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelOptionsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnOptionsClosedSignature OnOptionsClosed;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	virtual FReply NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	// UI Bindings
	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	TObjectPtr<USlider> Slider_Volume;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	TObjectPtr<USlider> Slider_Sensitivity;
    
	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	TObjectPtr<UEditableTextBox> Edit_Volume;
    
	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	TObjectPtr<UEditableTextBox> Edit_Sensitivity;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	TObjectPtr<UComboBoxString> Combo_ScreenMode;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	TObjectPtr<UComboBoxString> Combo_Resolution;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	TObjectPtr<UButton> Btn_Apply;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	TObjectPtr<UButton> Btn_Back;

	// Handlers
	UFUNCTION()
	void HandleVolumeChanged(float Value);

	UFUNCTION()
	void HandleSensitivityChanged(float Value);
	
	UFUNCTION()
	void HandleVolumeTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleSensitivityTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleApplyClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleSliderCaptureEnd();

private:
	void InitializeSettings();
	void PopulateScreenModeOptions();
	void PopulateResolutionOptions();
	void ApplySensitivityToOwningPawn(float Sensitivity);
	void SaveLocalSettingsIfDirty();

	bool bInitializingSettings = false;
	bool bLocalSettingsDirty = false;
};
