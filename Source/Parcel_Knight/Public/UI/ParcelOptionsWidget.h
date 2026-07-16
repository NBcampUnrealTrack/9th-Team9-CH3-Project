#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParcelOptionsWidget.generated.h"

class USlider;
class UComboBoxString;
class UButton;

/**
 * UParcelOptionsWidget
 * 볼륨, 마우스 감도 및 엔진 그래픽 세팅(해상도, 화면모드)을 조작하고 보관하는 위젯
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelOptionsWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// UI Bindings
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_Volume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_Sensitivity;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> Combo_ScreenMode;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> Combo_Resolution;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Apply;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Back;

	// Handlers
	UFUNCTION()
	void HandleVolumeChanged(float Value);

	UFUNCTION()
	void HandleSensitivityChanged(float Value);

	UFUNCTION()
	void HandleApplyClicked();

	UFUNCTION()
	void HandleBackClicked();

private:
	void InitializeSettings();
	void PopulateScreenModeOptions();
	void PopulateResolutionOptions();
};