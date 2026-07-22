#include "../../Public/UI/ParcelInGameDeadHUDWidget.h"
#include "Components/TextBlock.h"
#include "Core/ParcelPlayerController.h"
#include "TimerManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void UParcelInGameDeadHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UParcelInGameDeadHUDWidget::PlayHeartbeatSound()
{
	if (HeartbeatSound)
	{
		UGameplayStatics::PlaySound2D(this, HeartbeatSound);
	}
}

void UParcelInGameDeadHUDWidget::StartDeathCountdown(int32 TotalSeconds)
{
	CurrentCount = TotalSeconds;

	// UI 텍스트 초기화 및 첫 프레임 연출 트리거
	if (Txt_CountdownNumber)
	{
		Txt_CountdownNumber->SetText(FText::AsNumber(CurrentCount));
	}
	K2_OnCountdownChanged(CurrentCount);
	
	PlayHeartbeatSound();

	// 1초 간격으로 반복되는 로컬 타이머 구동
	GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(CountdownTimerHandle, this, &UParcelInGameDeadHUDWidget::AdvanceCountdown, 1.0f, true);
}

void UParcelInGameDeadHUDWidget::AdvanceCountdown()
{
	CurrentCount--;

	if (CurrentCount <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
        
		if (Txt_CountdownNumber)
		{
			Txt_CountdownNumber->SetText(FText::AsNumber(0));
		}
		K2_OnCountdownChanged(0);
        
		// 카운트가 다 끝나면 자동 리스폰 처리는 서버가 진행하므로 UI는 대기하네!
		return;
	}

	// 숫자가 바뀔 때마다 텍스트 갱신 및 블루프린트 애니메이션 이벤트 호출!
	if (Txt_CountdownNumber)
	{
		Txt_CountdownNumber->SetText(FText::AsNumber(CurrentCount));
	}
	K2_OnCountdownChanged(CurrentCount);
	
	PlayHeartbeatSound();
}

FReply UParcelInGameDeadHUDWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// [보완책 적용] UI Only 모드에서 ESC 키가 누락되는 트랩 방어
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (AParcelPlayerController* PC = Cast<AParcelPlayerController>(GetOwningPlayer()))
		{
			PC->ToggleInGameMenu();
			return FReply::Handled(); // 입력 처리 완료 선언
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}