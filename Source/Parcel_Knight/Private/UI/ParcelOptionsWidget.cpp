#include "UI/ParcelOptionsWidget.h"

#include "Character/ParcelHeroComponent.h"
#include "Components/Slider.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "GameFramework/GameUserSettings.h"

void UParcelOptionsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Slider_Volume)
        Slider_Volume->OnValueChanged.AddDynamic(this, &UParcelOptionsWidget::HandleVolumeChanged);
    
    if (Slider_Sensitivity)
        Slider_Sensitivity->OnValueChanged.AddDynamic(this, &UParcelOptionsWidget::HandleSensitivityChanged);
    
    if (Btn_Apply)
        Btn_Apply->OnClicked.AddDynamic(this, &UParcelOptionsWidget::HandleApplyClicked);
    
    if (Btn_Back)
        Btn_Back->OnClicked.AddDynamic(this, &UParcelOptionsWidget::HandleBackClicked);

    InitializeSettings();
}

void UParcelOptionsWidget::InitializeSettings()
{
    if (Slider_Volume)
        Slider_Volume->SetValue(0.8f);
    
    if (Slider_Sensitivity)
    {
        APawn* OwningPawn = GetOwningPlayerPawn();
        if (OwningPawn)
        {
            UParcelHeroComponent* HeroComp = OwningPawn->FindComponentByClass<UParcelHeroComponent>();
            if (HeroComp)
            {
                Slider_Sensitivity->SetValue(HeroComp->GetMouseSensitivity());
            }
        }
        else
        {
            Slider_Sensitivity->SetValue(1.0f);
        }
    }

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
    
    UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
    if (Settings)
    {
        EWindowMode::Type CurrentMode = Settings->GetFullscreenMode();
        switch (CurrentMode)
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

    UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
    if (Settings)
    {
        FIntPoint CurrentRes = Settings->GetScreenResolution();
        FString ResString = FString::Printf(TEXT("%dx%d"), CurrentRes.X, CurrentRes.Y);

        if (ResList.Contains(ResString))
        {
            Combo_Resolution->SetSelectedOption(ResString);
        }
        else
        {
            Combo_Resolution->SetSelectedOption(TEXT("1920x1080")); // Fallback 기본값
        }
    }
}

void UParcelOptionsWidget::HandleVolumeChanged(float Value)
{
    // TODO: SoundMix 및 SoundClass에 연동하여 마스터 볼륨을 조절하는 로직 추가
}

void UParcelOptionsWidget::HandleSensitivityChanged(float Value)
{
    APawn* OwningPawn = GetOwningPlayerPawn();
    if (OwningPawn)
    {
        UParcelHeroComponent* HeroComp = OwningPawn->FindComponentByClass<UParcelHeroComponent>();
        if (HeroComp)
        {
            HeroComp->SetMouseSensitivity(Value);
        }
    }
}

void UParcelOptionsWidget::HandleApplyClicked()
{
    UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
    if (!Settings) return;

    // 1. 화면 모드 적용
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

    // 2. 해상도 적용
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
}

void UParcelOptionsWidget::HandleBackClicked()
{
    RemoveFromParent();
}