#include "UI/ParcelHUDWidget.h"
#include "ParcelLog.h"
#include "GameFramework/GameStateBase.h"
#include "Core/ParcelGameState.h"
#include "Core/ParcelPlayerState.h"
#include "Core/TeamScoreComponent.h"
#include "Core/PlayerStatComponent.h"
#include "Character/ParcelInteractionComponent.h" 
#include "Core/HealthComponent.h"
#include "Character/CharacterCarryComponent.h"
#include "Delivery/DeliveryBox.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY(LogInGameHUD);

void UParcelHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TryBindUIEvents();
}

void UParcelHUDWidget::TryBindUIEvents()
{
	bool bInteractionBound = false;
	bool bHealthBound = false;
	bool bCarryBound = false;
	
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
				TeamScoreComp->OnTeamScoreChanged.AddDynamic(this, &UParcelHUDWidget::HandleOnTeamScoreChanged);
				HandleOnTeamScoreChanged(TeamScoreComp->GetTeamScore());

				// 라운드 만료 시간 강제 수신
				TeamScoreComp->OnRemainingTimeChanged.AddDynamic(this, &UParcelHUDWidget::HandleOnExpirationTimeChanged);
				HandleOnExpirationTimeChanged(TeamScoreComp->GetRemainingTime());
			}
		}
	}

	// 2. PlayerState 바인딩
	if (!CachedPlayerState.IsValid())
	{
		CachedPlayerState = Cast<AParcelPlayerState>(GetOwningPlayerState());

		if (CachedPlayerState.IsValid())
		{
			// Todo : 디커플링을 위해서 FindComponentByClass를 사용했습니다. 배포 버전을 만들 때 게터로 리팩토링이 필요합니다.
			UPlayerStatComponent* PlayerStatComp = CachedPlayerState->FindComponentByClass<UPlayerStatComponent>();

			// [이벤트 바인딩]
			PlayerStatComp->OnPersonalScoreChanged.AddDynamic(this, &UParcelHUDWidget::HandleOnPersonalScoreChanged);
			
			// 바인딩 성공시 현재 개인 점수로 UI 텍스트 초기화
			HandleOnPersonalScoreChanged(PlayerStatComp->GetPersonalScore());

			GetWorld()->GetTimerManager().ClearTimer(RetryBindTimerHandle);
		}
		else
		{
			if (!RetryBindTimerHandle.IsValid())
			{
				GetWorld()->GetTimerManager().SetTimer(RetryBindTimerHandle, this, &UParcelHUDWidget::TryBindUIEvents, 0.1f, true);
			}
		}
	}
	
	// 3. 컴포넌트 바인딩
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
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
	}
	
	// 4. 멀티플레이 안전장치
	if (CachedGameState.IsValid() && CachedPlayerState.IsValid() && bInteractionBound && bHealthBound && bCarryBound)
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

void UParcelHUDWidget::HandleOnPersonalScoreChanged(int32 NewPersonalScore)
{
	FText FormattedText = FText::Format(FText::FromString(TEXT("개인 점수 : {0}")), FText::AsNumber(NewPersonalScore));
	K2_OnPersonalScoreChanged(FormattedText);
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