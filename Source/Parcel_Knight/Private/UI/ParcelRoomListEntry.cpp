#include "UI/ParcelRoomListEntry.h"
#include "Components/TextBlock.h"
#include "UI/ParcelRoomListWidget.h"
#include "Core/SessionSubsystem.h"
#include "ParcelLog.h"

void UParcelRoomListEntry::NativeConstruct()
{
	Super::NativeConstruct();
}

void UParcelRoomListEntry::InitializeEntry(int32 InSessionIndex, UParcelRoomListWidget* InOwnerList)
{
	MySessionIndex = InSessionIndex;
	OwnerRoomListWidget = InOwnerList;

	USessionSubsystem* SS = GetGameInstance() ? GetGameInstance()->GetSubsystem<USessionSubsystem>() : nullptr;
	if (!SS) return;

	FString OwnerName = SS->GetSessionOwnerName(MySessionIndex);
	int32 CurrentPlayers = SS->GetSessionPlayerCount(MySessionIndex);
	
	FString FormattedRoomName = FString::Printf(TEXT("%s 의 방"), *OwnerName);
	FString FormattedPlayerCount = FString::Printf(TEXT("%d 명"), CurrentPlayers);

	if (Txt_RoomName) Txt_RoomName->SetText(FText::FromString(FormattedRoomName));
	if (Txt_PlayerCount) Txt_PlayerCount->SetText(FText::FromString(FormattedPlayerCount));
	
	K2_SetHighlightState(false);
}

void UParcelRoomListEntry::SelectThisRoom()
{
	if (OwnerRoomListWidget)
	{
		OwnerRoomListWidget->SetSelectedEntry(this);
	}
}

void UParcelRoomListEntry::JoinThisRoom()
{
	if (MySessionIndex == -1) return;

	USessionSubsystem* SS = GetGameInstance() ? GetGameInstance()->GetSubsystem<USessionSubsystem>() : nullptr;
	if (SS)
	{
		INGAMEHUD_LOG(Log, TEXT("[Room Entry] 세션 참가 시도 - 인덱스: %d"), MySessionIndex);
		SS->JoinSession(MySessionIndex);
	}
}