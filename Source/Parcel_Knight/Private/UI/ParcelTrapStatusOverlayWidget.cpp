#include "UI/ParcelTrapStatusOverlayWidget.h"

#include "Components/Border.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UParcelTrapStatusOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HideTrapStatus();
}

void UParcelTrapStatusOverlayWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
	}

	Super::NativeDestruct();
}

void UParcelTrapStatusOverlayWidget::ShowTrapStatus(FLinearColor Color, float Duration)
{
	UWorld* World = GetWorld();
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Trap UI] ShowTrapStatus called: Widget=%s Color RGBA=(%.3f, %.3f, %.3f, %.3f) Duration=%.3f TrapEdgeBorder valid=%d WorldValid=%d"),
		*GetNameSafe(this),
		Color.R,
		Color.G,
		Color.B,
		Color.A,
		Duration,
		IsValid(TrapEdgeBorder),
		IsValid(World)
	);

	if (!TrapEdgeBorder || !World || Duration <= 0.0f)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[Trap UI] ShowTrapStatus aborted: TrapEdgeBorder valid=%d WorldValid=%d Duration=%.3f"),
			IsValid(TrapEdgeBorder),
			IsValid(World),
			Duration
		);
		HideTrapStatus();
		return;
	}

	World->GetTimerManager().ClearTimer(HideTimerHandle);
	TrapEdgeBorder->SetBrushColor(Color);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Trap UI] ShowTrapStatus visible: Widget=%s Visibility=HitTestInvisible Color RGBA=(%.3f, %.3f, %.3f, %.3f) Duration=%.3f TrapEdgeBorder valid=1"),
		*GetNameSafe(this),
		Color.R,
		Color.G,
		Color.B,
		Color.A,
		Duration
	);
	World->GetTimerManager().SetTimer(
		HideTimerHandle,
		this,
		&UParcelTrapStatusOverlayWidget::HideTrapStatus,
		Duration,
		false
	);
}

void UParcelTrapStatusOverlayWidget::HideTrapStatus()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}
