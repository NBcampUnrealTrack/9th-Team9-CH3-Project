#include "UI/ParcelMainMenuWidget.h"
#include "Components/Button.h"
#include "UI/ParcelOptionsWidget.h"
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
	// 추후 상점 필요 시 여기다 연동
	INGAMEHUD_LOG(Warning, TEXT("[Main Menu] 상점 기능은 현재 프로토타입 단계에서 비활성화되어 있습니다."));
}

void UParcelMainMenuWidget::HandleExitGameClicked()
{
	INGAMEHUD_LOG(Log, TEXT("[Main Menu] 게임 종료 요청"));
	UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayer(), EQuitPreference::Quit, false);
}
