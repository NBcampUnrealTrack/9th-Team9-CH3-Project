#include "UI/ParcelMapSelectWidget.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "UI/ParcelMapEntryWidget.h"
#include "UI/ParcelLobbyHUDWidget.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void UParcelMapSelectWidget::BuildMapListPopulate(UDataTable* TargetDataTable, UParcelLobbyHUDWidget* OwnerHUD)
{
	if (!ScrollBox_MapList || !MapEntryClass || !TargetDataTable || !OwnerHUD) return;

	// 찌꺼기 방지 청소 가동
	ScrollBox_MapList->ClearChildren();

	// 데이터 테이블 통째로 긁어오기
	TArray<FParcelMapStageData*> AllMapRows;
	TargetDataTable->GetAllRows<FParcelMapStageData>(TEXT("DynamicMapSelectContext"), AllMapRows);

	// 루프 돌며 낱개 카드 팩토리 가동
	for (int32 i = 0; i < AllMapRows.Num(); ++i)
	{
		if (!AllMapRows[i]) continue;

		UParcelMapEntryWidget* NewCard = CreateWidget<UParcelMapEntryWidget>(GetOwningPlayer(), MapEntryClass);
		if (NewCard)
		{
			// 데이터와 소유 HUD 주소 주입
			NewCard->InitializeEntry(i, *AllMapRows[i], OwnerHUD);
			// 스크롤 박스의 자식으로 촤악 탑탑이 적재
			ScrollBox_MapList->AddChild(NewCard);
		}
	}

	// 닫기 버튼 동적 바인딩 안전 가드
	if (Btn_CloseMapMenu && !Btn_CloseMapMenu->OnClicked.IsBound())
	{
		Btn_CloseMapMenu->OnClicked.AddDynamic(this, &UParcelMapSelectWidget::HandleCloseClicked);
	}
}

void UParcelMapSelectWidget::HandleCloseClicked()
{
	PlayButtonClickSound();
	
	RemoveFromParent();
}

void UParcelMapSelectWidget::PlayButtonClickSound()
{
	if (ButtonClickSound)
	{
		// 뷰포트에 2D로 UI 효과음 출력
		UGameplayStatics::PlaySound2D(this, ButtonClickSound);
	}
}