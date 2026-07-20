#include "UI/ParcelOptionsWidget.h"
#include "Character/ParcelHeroComponent.h"
#include "Components/Slider.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "GameFramework/GameUserSettings.h"

void UParcelOptionsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Slider_Volume)
        Slider_Volume->OnValueChanged.AddDynamic(this, &UParcelOptionsWidget::HandleVolumeChanged);
    
    if (Slider_Sensitivity)
        Slider_Sensitivity->OnValueChanged.AddDynamic(this, &UParcelOptionsWidget::HandleSensitivityChanged);
    
    if (Edit_Volume)
        Edit_Volume->OnTextCommitted.AddDynamic(this, &UParcelOptionsWidget::HandleVolumeTextCommitted);

    if (Edit_Sensitivity)
        Edit_Sensitivity->OnTextCommitted.AddDynamic(this, &UParcelOptionsWidget::HandleSensitivityTextCommitted);
    
    if (Btn_Apply)
        Btn_Apply->OnClicked.AddDynamic(this, &UParcelOptionsWidget::HandleApplyClicked);
    
    if (Btn_Back)
        Btn_Back->OnClicked.AddDynamic(this, &UParcelOptionsWidget::HandleBackClicked);

    InitializeSettings();
}

void UParcelOptionsWidget::InitializeSettings()
{
    float DefaultVolume = 0.8f;
    if (Slider_Volume)
    {
        Slider_Volume->SetValue(DefaultVolume);
    }
    if (Edit_Volume)
    {
        Edit_Volume->SetText(FText::AsNumber(DefaultVolume));
    }
    
    float InitialSensitivity = 1.0f;
    
    if (APawn* OwningPawn = GetOwningPlayerPawn())
    {
        if (UParcelHeroComponent* HeroComp = OwningPawn->FindComponentByClass<UParcelHeroComponent>())
        {
            InitialSensitivity = HeroComp->GetMouseSensitivity();
        }
    }

    if (Slider_Sensitivity)
    {
        Slider_Sensitivity->SetValue(InitialSensitivity);
    }
    if (Edit_Sensitivity)
    {
        FNumberFormattingOptions FormatOptions;
        FormatOptions.MaximumFractionalDigits = 2;
        Edit_Sensitivity->SetText(FText::AsNumber(InitialSensitivity, &FormatOptions));
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
    if (Edit_Volume)
    {
        FNumberFormattingOptions FormatOptions;
        FormatOptions.MaximumFractionalDigits = 2;
        Edit_Volume->SetText(FText::AsNumber(Value, &FormatOptions));
    }

    // TODO: SoundMix 연동 로직
}

void UParcelOptionsWidget::HandleSensitivityChanged(float Value)
{
    if (Edit_Sensitivity)
    {
        FNumberFormattingOptions FormatOptions;
        FormatOptions.MaximumFractionalDigits = 2;
        Edit_Sensitivity->SetText(FText::AsNumber(Value, &FormatOptions));
    }

    if (APawn* OwningPawn = GetOwningPlayerPawn())
    {
        if (UParcelHeroComponent* HeroComp = OwningPawn->FindComponentByClass<UParcelHeroComponent>())
        {
            HeroComp->SetMouseSensitivity(Value);
        }
    }
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
    }
}

void UParcelOptionsWidget::HandleSensitivityTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus)
    {
        float NewValue = FCString::Atof(*Text.ToString());
        
        // 마우스 감도는 0이 될 수 없고 적정 최소/최대값 제한 (예: 0.05 ~ 10.0)
        NewValue = FMath::Clamp(NewValue, 0.05f, 10.f);

        if (Slider_Sensitivity)
        {
            Slider_Sensitivity->SetValue(NewValue);
        }

        HandleSensitivityChanged(NewValue);
    }
}

void UParcelOptionsWidget::HandleApplyClicked()
{
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
    }
}

void UParcelOptionsWidget::HandleBackClicked()
{
    RemoveFromParent();
}