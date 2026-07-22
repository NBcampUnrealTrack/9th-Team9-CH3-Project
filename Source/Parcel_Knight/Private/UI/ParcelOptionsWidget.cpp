#include "UI/ParcelOptionsWidget.h"
#include "Character/ParcelHeroComponent.h"
#include "Core/ParcelGameUserSettings.h"
#include "Components/Slider.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void UParcelOptionsWidget::NativeConstruct()
{
    Super::NativeConstruct();
	
	SetIsFocusable(true);

    if (Slider_Volume)
	{
		Slider_Volume->OnValueChanged.RemoveDynamic(this, &UParcelOptionsWidget::HandleVolumeChanged);
		Slider_Volume->OnMouseCaptureEnd.RemoveDynamic(this, &UParcelOptionsWidget::HandleSliderCaptureEnd);
		Slider_Volume->OnControllerCaptureEnd.RemoveDynamic(this, &UParcelOptionsWidget::HandleSliderCaptureEnd);
        Slider_Volume->OnValueChanged.AddDynamic(this, &UParcelOptionsWidget::HandleVolumeChanged);
		Slider_Volume->OnMouseCaptureEnd.AddDynamic(this, &UParcelOptionsWidget::HandleSliderCaptureEnd);
		Slider_Volume->OnControllerCaptureEnd.AddDynamic(this, &UParcelOptionsWidget::HandleSliderCaptureEnd);
	}
    
    if (Slider_Sensitivity)
	{
		Slider_Sensitivity->OnValueChanged.RemoveDynamic(this, &UParcelOptionsWidget::HandleSensitivityChanged);
		Slider_Sensitivity->OnMouseCaptureEnd.RemoveDynamic(this, &UParcelOptionsWidget::HandleSliderCaptureEnd);
		Slider_Sensitivity->OnControllerCaptureEnd.RemoveDynamic(this, &UParcelOptionsWidget::HandleSliderCaptureEnd);
        Slider_Sensitivity->OnValueChanged.AddDynamic(this, &UParcelOptionsWidget::HandleSensitivityChanged);
		Slider_Sensitivity->OnMouseCaptureEnd.AddDynamic(this, &UParcelOptionsWidget::HandleSliderCaptureEnd);
		Slider_Sensitivity->OnControllerCaptureEnd.AddDynamic(this, &UParcelOptionsWidget::HandleSliderCaptureEnd);
	}
    
    if (Edit_Volume)
	{
		Edit_Volume->OnTextCommitted.RemoveDynamic(this, &UParcelOptionsWidget::HandleVolumeTextCommitted);
        Edit_Volume->OnTextCommitted.AddDynamic(this, &UParcelOptionsWidget::HandleVolumeTextCommitted);
	}

    if (Edit_Sensitivity)
	{
		Edit_Sensitivity->OnTextCommitted.RemoveDynamic(this, &UParcelOptionsWidget::HandleSensitivityTextCommitted);
        Edit_Sensitivity->OnTextCommitted.AddDynamic(this, &UParcelOptionsWidget::HandleSensitivityTextCommitted);
	}
    
    if (Btn_Apply)
	{
		Btn_Apply->OnClicked.RemoveDynamic(this, &UParcelOptionsWidget::HandleApplyClicked);
        Btn_Apply->OnClicked.AddDynamic(this, &UParcelOptionsWidget::HandleApplyClicked);
	}
    
    if (Btn_Back)
	{
		Btn_Back->OnClicked.RemoveDynamic(this, &UParcelOptionsWidget::HandleBackClicked);
        Btn_Back->OnClicked.AddDynamic(this, &UParcelOptionsWidget::HandleBackClicked);
	}

    InitializeSettings();
}

void UParcelOptionsWidget::NativeDestruct()
{
	SaveLocalSettingsIfDirty();

	if (Slider_Volume)
	{
		Slider_Volume->OnValueChanged.RemoveDynamic(this, &UParcelOptionsWidget::HandleVolumeChanged);
		Slider_Volume->OnMouseCaptureEnd.RemoveDynamic(this, &UParcelOptionsWidget::HandleSliderCaptureEnd);
		Slider_Volume->OnControllerCaptureEnd.RemoveDynamic(this, &UParcelOptionsWidget::HandleSliderCaptureEnd);
	}

	if (Slider_Sensitivity)
	{
		Slider_Sensitivity->OnValueChanged.RemoveDynamic(this, &UParcelOptionsWidget::HandleSensitivityChanged);
		Slider_Sensitivity->OnMouseCaptureEnd.RemoveDynamic(this, &UParcelOptionsWidget::HandleSliderCaptureEnd);
		Slider_Sensitivity->OnControllerCaptureEnd.RemoveDynamic(this, &UParcelOptionsWidget::HandleSliderCaptureEnd);
	}

	if (Edit_Volume)
	{
		Edit_Volume->OnTextCommitted.RemoveDynamic(this, &UParcelOptionsWidget::HandleVolumeTextCommitted);
	}

	if (Edit_Sensitivity)
	{
		Edit_Sensitivity->OnTextCommitted.RemoveDynamic(this, &UParcelOptionsWidget::HandleSensitivityTextCommitted);
	}

	if (Btn_Apply)
	{
		Btn_Apply->OnClicked.RemoveDynamic(this, &UParcelOptionsWidget::HandleApplyClicked);
	}

	if (Btn_Back)
	{
		Btn_Back->OnClicked.RemoveDynamic(this, &UParcelOptionsWidget::HandleBackClicked);
	}

	Super::NativeDestruct();
}

void UParcelOptionsWidget::InitializeSettings()
{
	bInitializingSettings = true;

	float InitialVolume = 1.0f;
	float InitialSensitivity = 1.0f;
	if (const UParcelGameUserSettings* Settings = UParcelGameUserSettings::GetParcelGameUserSettings())
	{
		InitialVolume = Settings->GetMasterVolume();
		InitialSensitivity = Settings->GetMouseSensitivity();
	}

    if (Slider_Volume)
    {
		Slider_Volume->SetMinValue(0.0f);
		Slider_Volume->SetMaxValue(1.0f);
		Slider_Volume->SetValue(InitialVolume);
    }
    if (Edit_Volume)
    {
		FNumberFormattingOptions FormatOptions;
		FormatOptions.MinimumFractionalDigits = 2;
		FormatOptions.MaximumFractionalDigits = 2;
		Edit_Volume->SetText(FText::AsNumber(InitialVolume, &FormatOptions));
    }

    if (Slider_Sensitivity)
    {
		Slider_Sensitivity->SetMinValue(0.1f);
		Slider_Sensitivity->SetMaxValue(3.0f);
        Slider_Sensitivity->SetValue(InitialSensitivity);
    }
    if (Edit_Sensitivity)
    {
        FNumberFormattingOptions FormatOptions;
		FormatOptions.MinimumFractionalDigits = 1;
		FormatOptions.MaximumFractionalDigits = 1;
        Edit_Sensitivity->SetText(FText::AsNumber(InitialSensitivity, &FormatOptions));
    }

	ApplySensitivityToOwningPawn(InitialSensitivity);
	bInitializingSettings = false;
	bLocalSettingsDirty = false;

    PopulateScreenModeOptions();
    PopulateResolutionOptions();
}

void UParcelOptionsWidget::PopulateScreenModeOptions()
{
    if (!Combo_ScreenMode) return;

    Combo_ScreenMode->ClearOptions();
    Combo_ScreenMode->AddOption(TEXT("전체 화면"));
    Combo_ScreenMode->AddOption(TEXT("전체 창 모드"));
    Combo_ScreenMode->AddOption(TEXT("창 모드"));
    
    if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
    {
        switch (Settings->GetFullscreenMode())
        {
        case EWindowMode::Fullscreen:
            Combo_ScreenMode->SetSelectedOption(TEXT("전체 화면"));
            break;
        case EWindowMode::WindowedFullscreen:
            Combo_ScreenMode->SetSelectedOption(TEXT("전체 창 모드"));
            break;
        case EWindowMode::Windowed:
            Combo_ScreenMode->SetSelectedOption(TEXT("창 모드"));
            break;
        default:
            break;
        }
    }
}

void UParcelOptionsWidget::PopulateResolutionOptions()
{
    if (!Combo_Resolution) return;

    Combo_Resolution->ClearOptions();
    
    TArray<FString> ResList = {
        TEXT("1920x1080"),
        TEXT("1600x900"),
        TEXT("1280x720"),
        TEXT("2560x1440")
    };

    for (const FString& Res : ResList)
    {
        Combo_Resolution->AddOption(Res);
    }

    if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
    {
        FIntPoint CurrentRes = Settings->GetScreenResolution();
        FString ResString = FString::Printf(TEXT("%dx%d"), CurrentRes.X, CurrentRes.Y);

        if (ResList.Contains(ResString))
        {
            Combo_Resolution->SetSelectedOption(ResString);
        }
        else
        {
            Combo_Resolution->SetSelectedOption(TEXT("1920x1080"));
        }
    }
}

void UParcelOptionsWidget::HandleVolumeChanged(float Value)
{
	const float ClampedVolume = FMath::Clamp(Value, 0.0f, 1.0f);
    if (Edit_Volume)
    {
        FNumberFormattingOptions FormatOptions;
        FormatOptions.MaximumFractionalDigits = 2;
		FormatOptions.MinimumFractionalDigits = 2;
		Edit_Volume->SetText(FText::AsNumber(ClampedVolume, &FormatOptions));
    }

	if (UParcelGameUserSettings* Settings = UParcelGameUserSettings::GetParcelGameUserSettings())
	{
		Settings->SetMasterVolume(ClampedVolume);
		Settings->ApplyMasterVolume(this);
		bLocalSettingsDirty |= !bInitializingSettings;
	}
}

void UParcelOptionsWidget::HandleSensitivityChanged(float Value)
{
	const float ClampedSensitivity = FMath::Clamp(Value, 0.1f, 3.0f);
    if (Edit_Sensitivity)
    {
        FNumberFormattingOptions FormatOptions;
		FormatOptions.MinimumFractionalDigits = 1;
		FormatOptions.MaximumFractionalDigits = 1;
		Edit_Sensitivity->SetText(FText::AsNumber(ClampedSensitivity, &FormatOptions));
    }

	if (UParcelGameUserSettings* Settings = UParcelGameUserSettings::GetParcelGameUserSettings())
	{
		Settings->SetMouseSensitivity(ClampedSensitivity);
		bLocalSettingsDirty |= !bInitializingSettings;
	}

	ApplySensitivityToOwningPawn(ClampedSensitivity);
}

void UParcelOptionsWidget::HandleVolumeTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    // 유저가 엔터를 누르거나, 다른 곳을 눌러 포커스가 빠져나갔을 때만 반영
    if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus)
    {
        float NewValue = FCString::Atof(*Text.ToString());
        
        // 볼륨은 보통 0.0 ~ 1.0 사이로 작동하므로 안전하게 가두기
        NewValue = FMath::Clamp(NewValue, 0.f, 1.f);

        if (Slider_Volume)
        {
            Slider_Volume->SetValue(NewValue);
        }

        // 최종 확정된 수치로 텍스트 칸을 깨끗이 정리 (예: 999 입력 시 1.00으로 정정됨)
        HandleVolumeChanged(NewValue);
		SaveLocalSettingsIfDirty();
    }
}

void UParcelOptionsWidget::HandleSensitivityTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus)
    {
        float NewValue = FCString::Atof(*Text.ToString());
        
		// 마우스 감도는 프로젝트 설정 범위인 0.1 ~ 3.0으로 제한
		NewValue = FMath::Clamp(NewValue, 0.1f, 3.0f);

        if (Slider_Sensitivity)
        {
            Slider_Sensitivity->SetValue(NewValue);
        }

        HandleSensitivityChanged(NewValue);
		SaveLocalSettingsIfDirty();
    }
}

void UParcelOptionsWidget::HandleApplyClicked()
{
	PlayButtonClickSound();
	
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
    {
        if (Combo_ScreenMode)
        {
            FString SelectedMode = Combo_ScreenMode->GetSelectedOption();
            if (SelectedMode == TEXT("전체 화면"))
                Settings->SetFullscreenMode(EWindowMode::Fullscreen);
            else if (SelectedMode == TEXT("전체 창 모드"))
                Settings->SetFullscreenMode(EWindowMode::WindowedFullscreen);
            else if (SelectedMode == TEXT("창 모드"))
                Settings->SetFullscreenMode(EWindowMode::Windowed);
        }

        if (Combo_Resolution)
        {
            FString SelectedRes = Combo_Resolution->GetSelectedOption();
            FString LeftStr, RightStr;
            if (SelectedRes.Split(TEXT("x"), &LeftStr, &RightStr))
            {
                int32 Width = FCString::Atoi(*LeftStr);
                int32 Height = FCString::Atoi(*RightStr);
                Settings->SetScreenResolution(FIntPoint(Width, Height));
            }
        }
    
        Settings->ApplySettings(false);
		bLocalSettingsDirty = false;
    }

	SaveLocalSettingsIfDirty();
}

void UParcelOptionsWidget::HandleBackClicked()
{
	PlayButtonClickSound();
	
	SaveLocalSettingsIfDirty();
	
	if (OnOptionsClosed.IsBound())
	{
		OnOptionsClosed.Broadcast();
	}
	else
	{
		RemoveFromParent();
	}
}

void UParcelOptionsWidget::HandleSliderCaptureEnd()
{
	SaveLocalSettingsIfDirty();
}

void UParcelOptionsWidget::ApplySensitivityToOwningPawn(float Sensitivity)
{
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (UParcelHeroComponent* HeroComp = OwningPawn->FindComponentByClass<UParcelHeroComponent>())
		{
			HeroComp->SetMouseSensitivity(Sensitivity);
		}
	}
}

void UParcelOptionsWidget::SaveLocalSettingsIfDirty()
{
	if (!bLocalSettingsDirty)
	{
		return;
	}

	if (UParcelGameUserSettings* Settings = UParcelGameUserSettings::GetParcelGameUserSettings())
	{
		Settings->SaveSettings();
		bLocalSettingsDirty = false;
	}
}

FReply UParcelOptionsWidget::NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		return FReply::Unhandled();
	}

	return Super::NativeOnKeyDown(MyGeometry, InKeyEvent);
}

void UParcelOptionsWidget::PlayButtonClickSound()
{
	if (ButtonClickSound)
	{
		// 뷰포트에 2D로 UI 효과음 출력
		UGameplayStatics::PlaySound2D(this, ButtonClickSound);
	}
}