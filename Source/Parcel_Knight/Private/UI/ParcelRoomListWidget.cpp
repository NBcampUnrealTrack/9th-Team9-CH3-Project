#include "UI/ParcelRoomListWidget.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "UI/ParcelRoomListEntry.h"
#include "Core/SessionSubsystem.h"
#include "ParcelLog.h"

void UParcelRoomListWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (Btn_RefreshRooms)
		Btn_RefreshRooms->OnClicked.AddDynamic(this, &UParcelRoomListWidget::HandleRefreshRoomsClicked);

	if (Btn_BackToMenu)
		Btn_BackToMenu->OnClicked.AddDynamic(this, &UParcelRoomListWidget::HandleBackToMenuClicked);

	if (Btn_JoinRoom)
		Btn_JoinRoom->OnClicked.AddDynamic(this, &UParcelRoomListWidget::HandleJoinRoomClicked);
	
	if (Btn_CreateRoom)
		Btn_CreateRoom->OnClicked.AddDynamic(this, &UParcelRoomListWidget::HandleCreateRoomClicked);
}

void UParcelRoomListWidget::RefreshRoomList()
{
	if (!ScrollBox_Rooms) return;

	ScrollBox_Rooms->ClearChildren();
	CurrentlySelectedEntry = nullptr;

	USessionSubsystem* SS = GetGameInstance() ? GetGameInstance()->GetSubsystem<USessionSubsystem>() : nullptr;
	if (!SS || !RoomEntryClass) return;

	int32 RoomCount = SS->GetSearchResultCount();
	for (int32 i = 0; i < RoomCount; ++i)
	{
		if (UParcelRoomListEntry* NewRow = CreateWidget<UParcelRoomListEntry>(GetOwningPlayer(), RoomEntryClass))
		{
			NewRow->InitializeEntry(i, this);
			ScrollBox_Rooms->AddChild(NewRow);
		}
	}
}

void UParcelRoomListWidget::SetSelectedEntry(UParcelRoomListEntry* NewEntry)
{
	// 1. 기존에 선택되어 불이 켜져있던 녀석이 있다면 불을 꺼줌
	if (CurrentlySelectedEntry)
	{
		CurrentlySelectedEntry->K2_SetHighlightState(false);
	}

	// 2. 새로운 타겟 위젯 기억
	CurrentlySelectedEntry = NewEntry;

	// 3. 새로 선택된 녀석에게 "너 대장한테 간택 받았으니 불 켜라!" 하고 신호 전달
	if (CurrentlySelectedEntry)
	{
		CurrentlySelectedEntry->K2_SetHighlightState(true);
	}
}

void UParcelRoomListWidget::HandleRefreshRoomsClicked()
{
	INGAMEHUD_LOG(Log, TEXT("[Room List] 새로고침 버튼 클릭 -> 스팀 방 재검색 트리거"));
	USessionSubsystem* SS = GetGameInstance() ? GetGameInstance()->GetSubsystem<USessionSubsystem>() : nullptr;
	if (SS)
	{
		SS->FindSessions();
	}
}

void UParcelRoomListWidget::HandleBackToMenuClicked()
{
	INGAMEHUD_LOG(Log, TEXT("[Room List] 뒤로가기 버튼 클릭 -> 메인 메뉴 복귀 연출 개시"));
	K2_OnBackToMainMenuStarted(); // 블프단에 슬라이드 업 애니메이션 특명 하사
}

void UParcelRoomListWidget::HandleJoinRoomClicked()
{
	// 더블클릭 뿐만 아니라, 1번 클릭으로 강조된 상태에서 하단 버튼을 눌러도 입장 가능하게 함!
	if (CurrentlySelectedEntry)
	{
		INGAMEHUD_LOG(Log, TEXT("[Room List] 하단 버튼을 통해 선택된 세션 입장 처리"));
		CurrentlySelectedEntry->JoinThisRoom();
	}
	else
	{
		INGAMEHUD_LOG(Warning, TEXT("[Room List] 선택된 방이 없어 입장 버튼이 작동하지 않습니다."));
	}
}

void UParcelRoomListWidget::HandleCreateRoomClicked()
{
	INGAMEHUD_LOG(Log, TEXT("[Room List] 멀티플레이 방 만들기 버튼 클릭 -> 세션 생성 시퀀스 개시"));
	
	SetMapPath(TEXT("/Game/Maps/LV_DF_Lobby_Stage00"));
	
	CreateSession(4);
}