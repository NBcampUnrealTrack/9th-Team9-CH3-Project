#include "Delivery/DeliveryZone.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Delivery/DeliveryBox.h"
#include "Delivery/DeliverySubsystem.h"
#include "Core/ParcelGameState.h"
#include "Core/TeamScoreComponent.h"
#include "Core/ParcelPlayerState.h"
#include "GameFramework/Controller.h"

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

void ADeliveryZone::ProcessDelivery(AActor* InBox)
{
	ADeliveryBox* Box = Cast<ADeliveryBox>(InBox);
	if (!Box || !HasAuthority()) return;

	FBoxData Data = Box->GetBoxData();
	
	// 게임플레이태그 시스템으로 리팩토링 (조건문 간소화)
	bool bIsCorrectZone = Data.TargetZoneTag.MatchesTagExact(ZoneTag);
	
	int32 ScoreChange = bIsCorrectZone ? Data.BaseScore : Data.DamagePenalty;

	if (bIsCorrectZone)
	{
		DELIVERY_LOG(LogParcelDelivery, Log, TEXT("[Server] 배송지점에 도착. 점수 +%d"), ScoreChange);
		Box->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Delivered")));
	}
	else
	{
		DELIVERY_LOG(LogParcelDelivery, Warning, TEXT("[Server] 배송지점이 아님. 점수 페널티 %d"), ScoreChange);
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
	
	// Todo : 상자를 던져서 골인시키게 되면, 그 순간 Box->GetOwner()가 nullptr일수 있음. 해당 버그를 미리 차단할 필요.
	if (APawn* CarrierPawn = Cast<APawn>(Box->GetOwner()))
	{
		if (AController* CarrierController = CarrierPawn->GetController())
		{
			if (AParcelPlayerState* PlayerState = CarrierController->GetPlayerState<AParcelPlayerState>())
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
		}
	}
	
	// 사용 완료된 상자는 서브시스템을 통해 삭제
	if (UDeliverySubsystem* DeliverySubsystem = GetWorld()->GetSubsystem<UDeliverySubsystem>())
	{
		DeliverySubsystem->DespawnBox(Box);
	}
}