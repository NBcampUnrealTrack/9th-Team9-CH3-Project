#include "UI/ParcelMapEntryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "UI/ParcelLobbyHUDWidget.h"
#include "Engine/Texture2D.h"

void UParcelMapEntryWidget::InitializeEntry(int32 InIndex, const FParcelMapStageData& StageData, UParcelLobbyHUDWidget* InLobbyHUD)
{
	AssociatedMapIndex = InIndex;
	CachedLobbyHUD = InLobbyHUD;

	// 텍스트와 썸네일을 데이터 테이블 기반으로 정교하게 매핑
	if (Txt_EntryMapName)
	{
		Txt_EntryMapName->SetText(FText::FromString(StageData.StageName));
	}

	if (Img_EntryThumbnail)
	{
		UTexture2D* LoadedThumb = StageData.StageThumbnail.LoadSynchronous();
		if (LoadedThumb)
		{
			Img_EntryThumbnail->SetBrushFromTexture(LoadedThumb);
		}
	}

	// 버튼 클릭 이벤트 직결 연동
	if (Btn_SelectThisMap && !Btn_SelectThisMap->OnClicked.IsBound())
	{
		Btn_SelectThisMap->OnClicked.AddDynamic(this, &UParcelMapEntryWidget::HandleSelectThisMapClicked);
	}
}

void UParcelMapEntryWidget::HandleSelectThisMapClicked()
{
	if (CachedLobbyHUD)
	{
		CachedLobbyHUD->SelectMapByIndex(AssociatedMapIndex);
	}

	UWidget* CurrentParent = GetParent();
	while (CurrentParent && !CurrentParent->IsA(UUserWidget::StaticClass()))
	{
		CurrentParent = CurrentParent->GetParent();
	}
	
	if (CurrentParent)
	{
		CurrentParent->RemoveFromParent();
	}
}