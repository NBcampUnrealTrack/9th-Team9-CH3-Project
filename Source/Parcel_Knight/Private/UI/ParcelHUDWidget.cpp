#include "UI/ParcelHUDWidget.h"
#include "ParcelLog.h"
#include "GameFramework/GameStateBase.h"
#include "Core/ParcelGameState.h"
#include "Core/TeamScoreComponent.h"
#include "Character/ParcelInteractionComponent.h" 
#include "Core/HealthComponent.h"
#include "Character/CharacterCarryComponent.h"
#include "Character/ParcelCharacter.h"
#include "Character/ParcelHeroComponent.h"
#include "Character/ParcelPlayerStateComponent.h"
#include "Character/ParcelStaminaComponent.h"
#include "Components/Button.h"
#include "Delivery/DeliveryBox.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/ParcelFriendListWidget.h"

DEFINE_LOG_CATEGORY(LogInGameHUD);

void UParcelHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (FriendButton)
	{
		FriendButton->OnClicked.RemoveDynamic(this, &UParcelHUDWidget::ToggleFriendList);
		FriendButton->OnClicked.AddDynamic(this, &UParcelHUDWidget::ToggleFriendList);
	}

	TryBindUIEvents();
}

void UParcelHUDWidget::NativeDestruct()
{
	if (FriendButton)
	{
		FriendButton->OnClicked.RemoveDynamic(this, &UParcelHUDWidget::ToggleFriendList);
	}

	Super::NativeDestruct();
}

void UParcelHUDWidget::OpenFriendList()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController || !PlayerController->IsLocalController() || !FriendListWidgetClass)
	{
		return;
	}

	if (!FriendListWidgetInstance)
	{
		FriendListWidgetInstance = CreateWidget<UParcelFriendListWidget>(
			PlayerController,
			FriendListWidgetClass);
	}

	if (!FriendListWidgetInstance)
	{
		return;
	}

	if (!FriendListWidgetInstance->IsInViewport())
	{
		FriendListWidgetInstance->AddToViewport(100);
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(FriendListWidgetInstance->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;
}

void UParcelHUDWidget::ToggleFriendList()
{
	if (FriendListWidgetInstance && FriendListWidgetInstance->IsInViewport())
	{
		FriendListWidgetInstance->CloseFriendList();
		return;
	}

	OpenFriendList();
}

void UParcelHUDWidget::TryBindUIEvents()
{
	bool bInteractionBound = false;
	bool bHealthBound = false;
	bool bCarryBound = false;
	bool bComboBound = false;
	bool bCharacterStateBound = false;
	bool bHeroBound = false;
	bool bStaminaBound = false;
	bool bLogBound = false;
	
	// GameState와 TeamScore 바인딩
	if (!CachedGameState.IsValid())
	{
		CachedGameState = Cast<AParcelGameState>(GetWorld()->GetGameState());
		if (CachedGameState.IsValid())
		{
			// Todo : 디커플링을 위해서 FindComponentByClass를 사용했습니다. 배포 버전을 만들 때 게터로 리팩토링이 필요합니다.
			UTeamScoreComponent* TeamScoreComp = CachedGameState->FindComponentByClass<UTeamScoreComponent>();
			
			if (TeamScoreComp)
			{
				// 이벤트 바인딩
				TeamScoreComp->OnTeamScoreChanged.RemoveDynamic(this, &UParcelHUDWidget::HandleOnTeamScoreChanged);
				TeamScoreComp->OnTeamScoreChanged.AddDynamic(this, &UParcelHUDWidget::HandleOnTeamScoreChanged);
				HandleOnTeamScoreChanged(TeamScoreComp->GetTeamScore());
				
				// 콤보 시스템 바인딩
				TeamScoreComp->OnComboChanged.RemoveDynamic(this, &UParcelHUDWidget::HandleOnComboChanged);
				TeamScoreComp->OnComboChanged.AddDynamic(this, &UParcelHUDWidget::HandleOnComboChanged);
				HandleOnComboChanged(TeamScoreComp->GetComboCount());

				// 라운드 만료 시간 바인딩
				TeamScoreComp->OnRemainingTimeChanged.RemoveDynamic(this, &UParcelHUDWidget::HandleOnExpirationTimeChanged);
				TeamScoreComp->OnRemainingTimeChanged.AddDynamic(this, &UParcelHUDWidget::HandleOnExpirationTimeChanged);
				HandleOnExpirationTimeChanged(TeamScoreComp->GetRemainingTime());
				
				bComboBound = true;
			}
			
			CachedGameState->OnDeliveryLogReceived.RemoveDynamic(this, &UParcelHUDWidget::HandleOnDeliveryLogReceived);
			CachedGameState->OnDeliveryLogReceived.AddDynamic(this, &UParcelHUDWidget::HandleOnDeliveryLogReceived);
			bLogBound = true;
		}
	}

	// 2. PlayerState 바인딩(삭제)
	
	// 3. 컴포넌트 바인딩
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(OwningPawn))
		{
			if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
			{
				StateComp->OnCharacterStateTagsChanged.RemoveDynamic(this, &UParcelHUDWidget::HandleOnCharacterStateChanged);
				StateComp->OnCharacterStateTagsChanged.AddDynamic(this, &UParcelHUDWidget::HandleOnCharacterStateChanged);
             
				// 진입 시점의 최초 캐릭터 상태 태그 강제 초기화
				HandleOnCharacterStateChanged(StateComp->GetCharacterStateTags());
				bCharacterStateBound = true;
			}
		}
		
		// 상호작용
		if (UParcelInteractionComponent* InteractComp = OwningPawn->FindComponentByClass<UParcelInteractionComponent>())
		{
			InteractComp->OnFocusChanged.RemoveDynamic(this, &UParcelHUDWidget::HandleOnInteractionFocusChanged);
			InteractComp->OnFocusChanged.AddDynamic(this, &UParcelHUDWidget::HandleOnInteractionFocusChanged);
			HandleOnInteractionFocusChanged(InteractComp->GetCurrentFocusedActor());
			bInteractionBound = true;
		}

		// 체력
		if (UHealthComponent* HealthComp = OwningPawn->FindComponentByClass<UHealthComponent>())
		{
			HealthComp->OnHPChanged.RemoveDynamic(this, &UParcelHUDWidget::K2_OnHPChanged);
			HealthComp->OnHPChanged.AddDynamic(this, &UParcelHUDWidget::K2_OnHPChanged);
			K2_OnHPChanged(HealthComp->GetHP(), HealthComp->GetMaxHP());
			bHealthBound = true;
		}

		// 운반
		if (UCharacterCarryComponent* CarryComp = OwningPawn->FindComponentByClass<UCharacterCarryComponent>())
		{
			CarryComp->OnCarriedBoxChanged.RemoveDynamic(this, &UParcelHUDWidget::HandleOnCarriedBoxChanged);
			CarryComp->OnCarriedBoxChanged.AddDynamic(this, &UParcelHUDWidget::HandleOnCarriedBoxChanged);

			HandleOnCarriedBoxChanged(CarryComp->GetCarriedBox());
			bCarryBound = true;
		}
		
		// 던지기 차징 게이지
		if (UParcelHeroComponent* HeroComp = OwningPawn->FindComponentByClass<UParcelHeroComponent>())
		{
			HeroComp->OnThrowChargeChanged.RemoveDynamic(this, &UParcelHUDWidget::HandleOnThrowChargeChanged);
			HeroComp->OnThrowChargeChanged.AddDynamic(this, &UParcelHUDWidget::HandleOnThrowChargeChanged);
			HandleOnThrowChargeChanged(HeroComp->IsChargingThrow(), HeroComp->GetThrowChargeRatio());
			bHeroBound = true;
		}
		
		// 스태미나 바인딩
		if (UParcelStaminaComponent* StaminaComp = OwningPawn->FindComponentByClass<UParcelStaminaComponent>())
		{
			StaminaComp->OnStaminaChanged.RemoveDynamic(this, &UParcelHUDWidget::HandleOnStaminaChanged);
			StaminaComp->OnStaminaChanged.AddDynamic(this, &UParcelHUDWidget::HandleOnStaminaChanged);
			HandleOnStaminaChanged(StaminaComp->GetCurrentStamina(), StaminaComp->GetMaxStamina());
			bStaminaBound = true;
		}
	}
	
	// 4. 멀티플레이 안전장치
	if (CachedGameState.IsValid() && bInteractionBound && bHealthBound && bCarryBound && bComboBound && bCharacterStateBound && bHeroBound && bStaminaBound && bLogBound)
	{
		GetWorld()->GetTimerManager().ClearTimer(RetryBindTimerHandle);
		INGAMEHUD_LOG(Log, TEXT("[UI] 모든 인게임 HUD 요소가 안전하게 완전 결합되었습니다."));
	}
	else
	{
		if (!RetryBindTimerHandle.IsValid() && GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(RetryBindTimerHandle, this, &UParcelHUDWidget::TryBindUIEvents, 0.1f, true);
			INGAMEHUD_LOG(Warning, TEXT("[UI] 일부 액터 복제 대기 중. 0.1초 후 결합을 재시도합니다."));
		}
	}
}

// 핸들러 함수

void UParcelHUDWidget::HandleOnTeamScoreChanged(int32 NewTeamScore)
{
	FText FormattedText = FText::Format(FText::FromString(TEXT("팀 점수 : {0}")), FText::AsNumber(NewTeamScore));
	K2_OnTeamScoreChanged(FormattedText);
}

void UParcelHUDWidget::HandleOnComboChanged(int32 NewComboCount)
{
	if (NewComboCount > 0 && CachedGameState.IsValid())
	{
		UTeamScoreComponent* TeamScoreComp = CachedGameState->FindComponentByClass<UTeamScoreComponent>();
		if (TeamScoreComp)
		{
			float CurrentMultiplier = TeamScoreComp->GetComboMultiplier();
			
			int32 BonusPercent = FMath::RoundToInt((CurrentMultiplier - 1.0f) * 100.f);

			FText FormattedText;
			if (BonusPercent > 0)
			{
				FormattedText = FText::Format(
					FText::FromString(TEXT("연속 배송 성공! {0} Combo! (+{1}% 점수 보너스!)")), 
					FText::AsNumber(NewComboCount),
					FText::AsNumber(BonusPercent)
				);
			}
			else
			{
				FormattedText = FText::Format(
					FText::FromString(TEXT("연속 배송 성공! {0} Combo!")), 
					FText::AsNumber(NewComboCount)
				);
			}
			K2_OnComboChanged(NewComboCount, FormattedText);
			return;
		}
	}
	
	K2_OnComboChanged(0, FText::GetEmpty());
}

void UParcelHUDWidget::HandleOnExpirationTimeChanged(float NewExpirationTime)
{
	if (GetWorld() && NewExpirationTime > 0.0f)
	{
		CachedExpirationTime = GetWorld()->GetTimeSeconds() + NewExpirationTime;

		GetWorld()->GetTimerManager().ClearTimer(UILocalTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(UILocalTimerHandle, this, &UParcelHUDWidget::UpdateLocalTimer, 0.2f, true);
	}
}

void UParcelHUDWidget::HandleOnInteractionFocusChanged(AActor* NewFocusedActor)
{
	if (NewFocusedActor)
	{
		FText PromptText = FText::FromString(TEXT("E 키를 눌러 상호작용"));
		K2_OnCrosshairStateChanged(true, PromptText);
	}
	else
	{
		K2_OnCrosshairStateChanged(false, FText::GetEmpty());
	}
}

void UParcelHUDWidget::HandleOnCarriedBoxChanged(ADeliveryBox* NewCarriedBox)
{
	if (!NewCarriedBox)
	{
		K2_OnCarriedBoxInfoChanged(false, FText::GetEmpty(), FText::GetEmpty(), FGameplayTag());
		return;
	}
	
	FBoxData CarriedBoxData = NewCarriedBox->GetBoxData();
	
	FText BoxNameText = FText::FromString(CarriedBoxData.DisplayName);
	FText FormattedName = FText::Format(
		FText::FromString(TEXT("{0} ({1}kg)")), 
		BoxNameText, 
		FText::AsNumber(CarriedBoxData.Weight)
	);
	
	FText DestinationText = FText::FromString(TEXT("목적지 : 미지정 구역"));
	if (CarriedBoxData.TargetZoneTag.IsValid())
	{
		FString ZoneString = CarriedBoxData.TargetZoneTag.ToString();
		ZoneString.ReplaceInline(TEXT("Delivery."), TEXT(""));
		ZoneString.ReplaceInline(TEXT("Zone."), TEXT(""));
		
		DestinationText = FText::Format(
			FText::FromString(TEXT("목적지 : {0} 구역")), 
			FText::FromString(ZoneString)
		);
	}
	
	K2_OnCarriedBoxInfoChanged(true, FormattedName, DestinationText, CarriedBoxData.BoxTypeTag);
}

void UParcelHUDWidget::UpdateLocalTimer()
{
	float CurrentWorldTime = GetWorld()->GetTimeSeconds();
	float RawRemainingTime = CachedExpirationTime - CurrentWorldTime;

	if (RawRemainingTime <= 0.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(UILocalTimerHandle);
		RawRemainingTime = 0.0f;
	}
	
	// Min Sec 쪼개기
	int32 TotalSeconds = FMath::FloorToInt(RawRemainingTime);
	int32 Minutes = TotalSeconds / 60;
	int32 Seconds = TotalSeconds % 60;
	
	// 언리얼 내부 함수를 사용해서 2자리 수 맞추기
	FNumberFormattingOptions TwoDigitOptions;
	TwoDigitOptions.MinimumIntegralDigits = 2;
	
	FText MinText = FText::AsNumber(Minutes, &TwoDigitOptions);
	FText SecText = FText::AsNumber(Seconds, &TwoDigitOptions);
	
	// Format Text 문자열 조립
	FText FormattedTime = FText::Format(
		FText::FromString(TEXT("남은시간 {0} : {1}")), 
		MinText, 
		SecText
	);
	
	K2_OnRemainingTimeChanged(FormattedTime, RawRemainingTime);
}

void UParcelHUDWidget::HandleOnCharacterStateChanged(const FGameplayTagContainer& ActiveTags)
{
	K2_OnCharacterStateChanged(ActiveTags);
}

void UParcelHUDWidget::HandleOnThrowChargeChanged(bool bIsCharging, float ChargeRatio)
{
	K2_OnThrowChargeChanged(bIsCharging, ChargeRatio);
}

void UParcelHUDWidget::HandleOnStaminaChanged(float CurrentStamina, float MaxStamina)
{
	K2_OnStaminaChanged(CurrentStamina, MaxStamina);
}

void UParcelHUDWidget::HandleOnDeliveryLogReceived(const FString& PlayerName, const FString& BoxName, bool bSuccess)
{
	K2_OnDeliveryLogAdded(PlayerName, BoxName, bSuccess);
}
