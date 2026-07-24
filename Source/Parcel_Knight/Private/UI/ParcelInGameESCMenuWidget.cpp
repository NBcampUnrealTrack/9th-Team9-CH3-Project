#include "UI/ParcelInGameESCMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Engine/GameInstance.h"
#include "Core/SessionSubsystem.h"
#include "Core/ParcelPlayerState.h"
#include "UI/ParcelOptionsWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void UParcelInGameESCMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (Btn_Resume)
		Btn_Resume->OnClicked.AddDynamic(this, &UParcelInGameESCMenuWidget::HandleResumeClicked);
    
	if (Btn_Restart)
		Btn_Restart->OnClicked.AddDynamic(this, &UParcelInGameESCMenuWidget::HandleRestartClicked);
    
	if (Btn_Options)
		Btn_Options->OnClicked.AddDynamic(this, &UParcelInGameESCMenuWidget::HandleOptionsClicked);
    
	if (Btn_Exit)
		Btn_Exit->OnClicked.AddDynamic(this, &UParcelInGameESCMenuWidget::HandleExitClicked);
}

void UParcelInGameESCMenuWidget::NativeDestruct()
{
	CloseOptionsWidget();
	Super::NativeDestruct();
}

FReply UParcelInGameESCMenuWidget::NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (OptionsWidgetInstance && OptionsWidgetInstance->IsInViewport())
		{
			CloseOptionsWidget();
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(MyGeometry, InKeyEvent);
}

void UParcelInGameESCMenuWidget::SetupMenu()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;

	if (Btn_Restart)
	{
		if (PC->HasAuthority()) Btn_Restart->SetIsEnabled(true);
		else Btn_Restart->SetIsEnabled(false);
	}

	RefreshMenuData();
	
	K2_OnMenuOpenStarted();
}

void UParcelInGameESCMenuWidget::CompleteTeardown()
{
	CloseOptionsWidget();
	
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}

	RemoveFromParent();
}

void UParcelInGameESCMenuWidget::RefreshMenuData()
{
	UpdateMapName();
	UpdatePersonalScore();
	UpdatePlayerList();
}

void UParcelInGameESCMenuWidget::UpdateMapName()
{
	if (!Txt_MapName) return;
	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	Txt_MapName->SetText(FText::FromString(MapName));
}

void UParcelInGameESCMenuWidget::UpdatePersonalScore()
{
	if (!Txt_PersonalScore) return;
	AParcelPlayerState* PS = Cast<AParcelPlayerState>(GetOwningPlayerState());
	if (PS)
	{
		int32 Score = PS->GetPersonalScore();
		Txt_PersonalScore->SetText(FText::Format(FText::FromString(TEXT("내 점수: {0}점")), FText::AsNumber(Score)));
	}
	else Txt_PersonalScore->SetText(FText::FromString(TEXT("내 점수: 0점")));
}

void UParcelInGameESCMenuWidget::UpdatePlayerList()
{
	if (!VB_PlayerList) return;
	VB_PlayerList->ClearChildren();

	AGameStateBase* GS = GetWorld()->GetGameState();
	if (!GS) return;
    
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (PS)
		{
			UTextBlock* PlayerNameText = NewObject<UTextBlock>(this);
			if (PlayerNameText)
			{
				PlayerNameText->SetText(FText::FromString(PS->GetPlayerName()));
				FSlateFontInfo FontInfo = PlayerNameText->GetFont();
				FontInfo.Size = 16.f;
				PlayerNameText->SetFont(FontInfo);
				VB_PlayerList->AddChild(PlayerNameText);
			}
		}
	}
}

void UParcelInGameESCMenuWidget::HandleResumeClicked()
{
	PlayButtonClickSound();
	
	K2_OnMenuCloseStarted();
}

void UParcelInGameESCMenuWidget::HandleRestartClicked()
{
	PlayButtonClickSound();
	
	APlayerController* PC = GetOwningPlayer();
	if (PC && PC->HasAuthority())
	{
		FString CurrentMapName = GetWorld()->GetMapName();
		CurrentMapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
		GetWorld()->ServerTravel(CurrentMapName + "?listen");
	}
}

void UParcelInGameESCMenuWidget::HandleOptionsClicked()
{
	PlayButtonClickSound();
	
	if (OptionsWidgetInstance && OptionsWidgetInstance->IsInViewport())
	{
		CloseOptionsWidget();
		return;
	}

	if (OptionsWidgetClass)
	{
		OptionsWidgetInstance = CreateWidget<UParcelOptionsWidget>(GetOwningPlayer(), OptionsWidgetClass);
		if (OptionsWidgetInstance)
		{
			OptionsWidgetInstance->OnOptionsClosed.RemoveDynamic(this, &UParcelInGameESCMenuWidget::CloseOptionsWidget);
			OptionsWidgetInstance->OnOptionsClosed.AddDynamic(this, &UParcelInGameESCMenuWidget::CloseOptionsWidget);

			OptionsWidgetInstance->AddToViewport(600);
          
			APlayerController* PC = GetOwningPlayer();
			if (PC)
			{
				FInputModeGameAndUI InputMode;
				InputMode.SetWidgetToFocus(OptionsWidgetInstance->TakeWidget());
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				PC->SetInputMode(InputMode);
			}
		}
	}
}

void UParcelInGameESCMenuWidget::CloseOptionsWidget()
{
	if (OptionsWidgetInstance)
	{
		OptionsWidgetInstance->OnOptionsClosed.RemoveDynamic(this, &UParcelInGameESCMenuWidget::CloseOptionsWidget);

		if (OptionsWidgetInstance->IsInViewport())
		{
			OptionsWidgetInstance->RemoveFromParent();
		}
		OptionsWidgetInstance = nullptr;
		
		APlayerController* PC = GetOwningPlayer();
		if (PC)
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
		}
	}
}

void UParcelInGameESCMenuWidget::HandleExitClicked()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<USessionSubsystem>())
		{
			K2_OnTeardownStarted();
			SessionSubsystem->LeaveSession();
		}
	}
}

bool UParcelInGameESCMenuWidget::CloseSubMenuIfOpen()
{
	if (OptionsWidgetInstance && OptionsWidgetInstance->IsInViewport())
	{
		CloseOptionsWidget();
		return true;
	}
	return false;
}

void UParcelInGameESCMenuWidget::PlayButtonClickSound()
{
	if (ButtonClickSound)
	{
		// 뷰포트에 2D로 UI 효과음 출력
		UGameplayStatics::PlaySound2D(this, ButtonClickSound);
	}
}