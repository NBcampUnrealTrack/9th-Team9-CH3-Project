#include "UI/ParcelMainMenuWidget.h"
#include "Components/Button.h"
#include "UI/ParcelOptionsWidget.h"
#include "UI/ParcelShopInventoryWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ParcelLog.h"

void UParcelMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_SinglePlay)
	{
		Btn_SinglePlay->OnClicked.RemoveDynamic(this, &UParcelMainMenuWidget::HandleSinglePlayClicked);
		Btn_SinglePlay->OnClicked.AddDynamic(this, &UParcelMainMenuWidget::HandleSinglePlayClicked);
	}

	if (Btn_MultiPlay)
	{
		Btn_MultiPlay->OnClicked.RemoveDynamic(this, &UParcelMainMenuWidget::HandleMultiPlayClicked);
		Btn_MultiPlay->OnClicked.AddDynamic(this, &UParcelMainMenuWidget::HandleMultiPlayClicked);
	}

	if (Btn_Options)
	{
		Btn_Options->OnClicked.RemoveDynamic(this, &UParcelMainMenuWidget::HandleOptionsClicked);
		Btn_Options->OnClicked.AddDynamic(this, &UParcelMainMenuWidget::HandleOptionsClicked);
	}

	if (Btn_Shop)
	{
		Btn_Shop->OnClicked.RemoveDynamic(this, &UParcelMainMenuWidget::HandleShopClicked);
		Btn_Shop->OnClicked.AddDynamic(this, &UParcelMainMenuWidget::HandleShopClicked);
	}

	if (Btn_ExitGame)
	{
		Btn_ExitGame->OnClicked.RemoveDynamic(this, &UParcelMainMenuWidget::HandleExitGameClicked);
		Btn_ExitGame->OnClicked.AddDynamic(this, &UParcelMainMenuWidget::HandleExitGameClicked);
	}
}

void UParcelMainMenuWidget::NativeDestruct()
{
	if (ShopInventoryWidgetInstance)
	{
		ShopInventoryWidgetInstance->OnCloseRequested.RemoveDynamic(
			this,
			&UParcelMainMenuWidget::HandleShopCloseRequested);
		ShopInventoryWidgetInstance->RemoveFromParent();
		ShopInventoryWidgetInstance = nullptr;
	}

	if (Btn_SinglePlay)
	{
		Btn_SinglePlay->OnClicked.RemoveDynamic(this, &UParcelMainMenuWidget::HandleSinglePlayClicked);
	}
	if (Btn_MultiPlay)
	{
		Btn_MultiPlay->OnClicked.RemoveDynamic(this, &UParcelMainMenuWidget::HandleMultiPlayClicked);
	}
	if (Btn_Options)
	{
		Btn_Options->OnClicked.RemoveDynamic(this, &UParcelMainMenuWidget::HandleOptionsClicked);
	}
	if (Btn_Shop)
	{
		Btn_Shop->OnClicked.RemoveDynamic(this, &UParcelMainMenuWidget::HandleShopClicked);
	}
	if (Btn_ExitGame)
	{
		Btn_ExitGame->OnClicked.RemoveDynamic(this, &UParcelMainMenuWidget::HandleExitGameClicked);
	}

	Super::NativeDestruct();
}

void UParcelMainMenuWidget::HandleSinglePlayClicked()
{
	INGAMEHUD_LOG(Log, TEXT("[Main Menu] 싱글 플레이 모드 진입 시작"));

	// HasAuthority 1인 리슨 서버 구동
	// [중요] 실제 마무리 단계에서는 맵 이름 정확하게 세팅해서 경로 바꿔줘야 함
	SetMapPath(TEXT("/Game/Maps/TestMaps/Testing_DF_Stage01"));
	
	CreateSession(1);
}

void UParcelMainMenuWidget::HandleMultiPlayClicked()
{
	INGAMEHUD_LOG(Log, TEXT("[Main Menu] 멀티 플레이 버튼 클릭"));

	// 스팀 서버에 방 목록 요청
	FindSessions();
	
	K2_OnMultiPlayMenuOpened();
}

void UParcelMainMenuWidget::HandleOptionsClicked()
{
	if (!OptionsWidgetClass)
	{
		INGAMEHUD_LOG(Error, TEXT("[Main Menu] OptionsWidgetClass가 할당되지 않았습니다."));
		return;
	}

	// OptionsWidget
	if (UParcelOptionsWidget* OptionsMenu = CreateWidget<UParcelOptionsWidget>(GetOwningPlayer(), OptionsWidgetClass))
	{
		OptionsMenu->AddToViewport(110);
	}
}

void UParcelMainMenuWidget::HandleShopClicked()
{
	if (!ShopInventoryWidgetClass)
	{
		INGAMEHUD_LOG(Error, TEXT("[Main Menu] ShopInventoryWidgetClass is not assigned."));
		return;
	}

	if (!ShopInventoryWidgetInstance)
	{
		ShopInventoryWidgetInstance = CreateWidget<UParcelShopInventoryWidget>(
			GetOwningPlayer(),
			ShopInventoryWidgetClass);
		if (!ShopInventoryWidgetInstance)
		{
			INGAMEHUD_LOG(Error, TEXT("[Main Menu] Failed to create the shop inventory widget."));
			return;
		}

		ShopInventoryWidgetInstance->OnCloseRequested.RemoveDynamic(
			this,
			&UParcelMainMenuWidget::HandleShopCloseRequested);
		ShopInventoryWidgetInstance->OnCloseRequested.AddDynamic(
			this,
			&UParcelMainMenuWidget::HandleShopCloseRequested);
		ShopInventoryWidgetInstance->AddToViewport(120);
	}

	ShopInventoryWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	ShopInventoryWidgetInstance->RefreshShopUI();
	SetVisibility(ESlateVisibility::Collapsed);

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ShopInventoryWidgetInstance->TakeWidget());
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}
}

void UParcelMainMenuWidget::HandleShopCloseRequested()
{
	if (ShopInventoryWidgetInstance)
	{
		ShopInventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetVisibility(ESlateVisibility::Visible);
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}
}

void UParcelMainMenuWidget::HandleExitGameClicked()
{
	INGAMEHUD_LOG(Log, TEXT("[Main Menu] 게임 종료 요청"));
	UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayer(), EQuitPreference::Quit, false);
}
