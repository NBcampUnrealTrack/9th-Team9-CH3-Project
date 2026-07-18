#include "Core/ParcelPlayerController.h"
#include "Core/ParcelCheatManager.h"
#include "Core/HealthComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"

AParcelPlayerController::AParcelPlayerController()
{
	CheatClass = UParcelCheatManager::StaticClass();
}

void AParcelPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;

		if (HUDWidgetClass)
		{
			HUDWidgetInstance = CreateWidget<UUserWidget>(this, HUDWidgetClass);
			if (HUDWidgetInstance)
				HUDWidgetInstance->AddToViewport();
		}
	}
}

void AParcelPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}

// ========================= 관전 =========================

void AParcelPlayerController::StartSpectating()
{
	bIsSpectating = true;
	SpectatorTargetIndex = 0;

	TArray<APawn*> AlivePawns = GetAlivePawns();
	if (AlivePawns.Num() > 0)
		SetViewTarget(AlivePawns[0]);
	// 살아있는 플레이어 없으면 현재 시야 그대로 유지
}

void AParcelPlayerController::SpectateNext()
{
	TArray<APawn*> AlivePawns = GetAlivePawns();
	if (AlivePawns.Num() == 0) return;

	SpectatorTargetIndex = (SpectatorTargetIndex + 1) % AlivePawns.Num();
	SetViewTarget(AlivePawns[SpectatorTargetIndex]);
}

void AParcelPlayerController::SpectatePrev()
{
	TArray<APawn*> AlivePawns = GetAlivePawns();
	if (AlivePawns.Num() == 0) return;

	SpectatorTargetIndex = (SpectatorTargetIndex - 1 + AlivePawns.Num()) % AlivePawns.Num();
	SetViewTarget(AlivePawns[SpectatorTargetIndex]);
}

TArray<APawn*> AParcelPlayerController::GetAlivePawns() const
{
	TArray<APawn*> AlivePawns;
	AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GS) return AlivePawns;

	for (APlayerState* PS : GS->PlayerArray)
	{
		if (PS == PlayerState) continue;
		APawn* TargetPawn = PS->GetPawn();
		if (!IsValid(TargetPawn)) continue;

		UHealthComponent* HC = TargetPawn->FindComponentByClass<UHealthComponent>();
		if (HC && !HC->IsDead())
			AlivePawns.Add(TargetPawn);
	}
	return AlivePawns;
}

bool AParcelPlayerController::InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	if (bIsSpectating)
	{
		// 왼쪽 클릭만 허용 — 다음 관전 대상으로 이동
		if (EventType == IE_Pressed && Key == EKeys::LeftMouseButton)
			SpectateNext();

		return true; // 나머지 입력 전부 차단
	}
	return Super::InputKey(Key, EventType, AmountDepressed, bGamepad);
}

void AParcelPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	bIsSpectating = false;
}

void AParcelPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);
	bIsSpectating = false;
	if (P)
		SetViewTarget(P);
}
