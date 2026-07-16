#include "UI/ParcelInGameESCMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Core/ParcelPlayerState.h"

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
	K2_OnMenuCloseStarted();
}

void UParcelInGameESCMenuWidget::HandleRestartClicked()
{
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
}

void UParcelInGameESCMenuWidget::HandleExitClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	if (PC->HasAuthority()) UGameplayStatics::OpenLevel(GetWorld(), TEXT("MainMenu"));
	else PC->ClientTravel(TEXT("/Game/Maps/MainMenu"), TRAVEL_Absolute);
}
