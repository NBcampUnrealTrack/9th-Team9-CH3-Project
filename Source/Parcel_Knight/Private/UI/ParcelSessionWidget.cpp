#include "UI/ParcelSessionWidget.h"
#include "Core/SessionSubsystem.h"
#include "Core/ParcelGameInstance.h"

void UParcelSessionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (USessionSubsystem* SS = GetSessionSubsystem())
	{
		SS->OnSessionCreateComplete.RemoveDynamic(this, &UParcelSessionWidget::HandleSessionCreateComplete);
		SS->OnSessionFindComplete.RemoveDynamic(this, &UParcelSessionWidget::HandleSessionFindComplete);
		SS->OnSessionJoinComplete.RemoveDynamic(this, &UParcelSessionWidget::HandleSessionJoinComplete);
		SS->OnSessionStatusMessage.RemoveDynamic(this, &UParcelSessionWidget::HandleSessionStatusMessage);
		SS->OnSessionCreateComplete.AddDynamic(this, &UParcelSessionWidget::HandleSessionCreateComplete);
		SS->OnSessionFindComplete.AddDynamic(this, &UParcelSessionWidget::HandleSessionFindComplete);
		SS->OnSessionJoinComplete.AddDynamic(this, &UParcelSessionWidget::HandleSessionJoinComplete);
		SS->OnSessionStatusMessage.AddDynamic(this, &UParcelSessionWidget::HandleSessionStatusMessage);
	}
}

void UParcelSessionWidget::NativeDestruct()
{
	if (USessionSubsystem* SS = GetSessionSubsystem())
	{
		SS->OnSessionCreateComplete.RemoveDynamic(this, &UParcelSessionWidget::HandleSessionCreateComplete);
		SS->OnSessionFindComplete.RemoveDynamic(this, &UParcelSessionWidget::HandleSessionFindComplete);
		SS->OnSessionJoinComplete.RemoveDynamic(this, &UParcelSessionWidget::HandleSessionJoinComplete);
		SS->OnSessionStatusMessage.RemoveDynamic(this, &UParcelSessionWidget::HandleSessionStatusMessage);
	}

	Super::NativeDestruct();
}

void UParcelSessionWidget::CreateSession(int32 NumPlayers)
{
	if (USessionSubsystem* SS = GetSessionSubsystem())
		SS->CreateSession(NumPlayers);
}

void UParcelSessionWidget::FindSessions()
{
	if (USessionSubsystem* SS = GetSessionSubsystem())
		SS->FindSessions();
}

void UParcelSessionWidget::JoinSession(int32 SessionIndex)
{
	if (USessionSubsystem* SS = GetSessionSubsystem())
		SS->JoinSession(SessionIndex);
}

void UParcelSessionWidget::SetMapPath(const FString& MapPath)
{
	if (UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>())
		GI->SetPendingMapPath(MapPath);
}

void UParcelSessionWidget::HandleSessionCreateComplete(bool bWasSuccessful)
{
	OnSessionCreated(bWasSuccessful);
}

void UParcelSessionWidget::HandleSessionFindComplete(bool bWasSuccessful)
{
	OnSessionsFound(bWasSuccessful);
}

void UParcelSessionWidget::HandleSessionJoinComplete(bool bWasSuccessful)
{
	OnSessionJoined(bWasSuccessful);
}

void UParcelSessionWidget::HandleSessionStatusMessage(const FText& Message, bool bIsError)
{
	OnSessionStatusChanged(Message, bIsError);
}

USessionSubsystem* UParcelSessionWidget::GetSessionSubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<USessionSubsystem>() : nullptr;
}
