#include "Delivery/DeliveryZone.h"
#include "ParcelLog.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Delivery/DeliveryBox.h"
#include "Delivery/DeliverySubsystem.h"
#include "Core/ParcelGameState.h"
#include "Core/TeamScoreComponent.h"
#include "Core/ParcelPlayerState.h"
#include "GameFramework/Controller.h"

DEFINE_LOG_CATEGORY(LogDeliveryZone);

ADeliveryZone::ADeliveryZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneMesh"));
	RootComponent = ZoneMesh;

	OverlapVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapVolume"));
	OverlapVolume->SetupAttachment(RootComponent);
	OverlapVolume->SetCollisionProfileName(TEXT("Trigger"));
}

void ADeliveryZone::BeginPlay()
{
	Super::BeginPlay();
	
	// OverlapVolume이 true인지도 미리 체크
	if (HasAuthority() && OverlapVolume)
	{
		OverlapVolume->OnComponentBeginOverlap.AddDynamic(this, &ADeliveryZone::OnZoneOverlap);
	}
}

void ADeliveryZone::OnZoneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
								  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
								  bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !OtherActor) return;

	if (ADeliveryBox* Box = Cast<ADeliveryBox>(OtherActor))
	{
		if (Box->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Delivered")))) return;

		ProcessDelivery(Box);
	}
}

void ADeliveryZone::ProcessDelivery(ADeliveryBox* Box)
{
	if (!Box || !HasAuthority()) return;

	FBoxData Data = Box->GetBoxData();
	
	// 게임플레이태그 시스템으로 리팩토링 (조건문 간소화)
	bool bIsCorrectZone = Data.TargetZoneTag.MatchesTagExact(ZoneTag);
	
	// [검토] : DamagePenalty를 데이터 테이블에서 입력할 때, 반드시 음수 (-) 값으로 붙여야 정상적으로 점수가 계산됨 (by JYW)
	int32 ScoreChange = bIsCorrectZone ? Data.BaseScore : Data.DamagePenalty;

	if (bIsCorrectZone)
	{
		DELIVERYZONE_LOG(Log, TEXT("[Server] 배송지점에 도착. 점수 +%d"), ScoreChange);
		Box->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Delivered")));
	}
	else
	{
		DELIVERYZONE_LOG(Warning, TEXT("[Server] 배송지점이 아님. 점수 페널티 %d"), ScoreChange);
		Box->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Failed")));
	}
	
	// 팀 점수
	if (AParcelGameState* GameState = GetWorld()->GetGameState<AParcelGameState>())
	{
		if (UTeamScoreComponent* TeamScoreComp = GameState->GetTeamScoreComponent())
		{
			TeamScoreComp->AddTeamScore(ScoreChange);
		}
	}
	
	// 상자를 쥐고 골인했거나 던져서 골인한 경우 모두 득점 처리하기 위해 최근 운반자 상태를 확인합니다.
	AParcelPlayerState* PlayerState = nullptr;
	if (APawn* CarrierPawn = Cast<APawn>(Box->GetOwner()))
	{
		if (AController* CarrierController = CarrierPawn->GetController())
		{
			PlayerState = CarrierController->GetPlayerState<AParcelPlayerState>();
		}
	}
	
	// 던져진 상자라 소유자가 nullptr인 경우 상자에 보관해둔 약한 참조를 통해 최근 배송 플레이어 상태를 획득
	if (!PlayerState && Box->GetLastCarrierPlayerState().IsValid())
	{
		PlayerState = Box->GetLastCarrierPlayerState().Get();
	}

	if (PlayerState)
	{
		PlayerState->AddScore(ScoreChange);
		if (bIsCorrectZone)
		{
			PlayerState->OnDeliverySuccess();
		}
		else
		{
			PlayerState->OnDeliveryFail();
		}
	}
	
	// 사용 완료된 상자는 서브시스템을 통해 삭제
	if (UDeliverySubsystem* DeliverySubsystem = GetWorld()->GetSubsystem<UDeliverySubsystem>())
	{
		DeliverySubsystem->DespawnBox(Box);
	}
}