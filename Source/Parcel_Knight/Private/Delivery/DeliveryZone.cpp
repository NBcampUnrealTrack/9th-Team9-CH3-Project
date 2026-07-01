#include "Delivery/DeliveryZone.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Delivery/DeliveryBox.h"
#include "Delivery/PhysicsJudgeManager.h"
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
	
	ZoneType = EDestinationType::None;
}

void ADeliveryZone::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		OverlapVolume->OnComponentBeginOverlap.AddDynamic(this, &ADeliveryZone::OnZoneOverlap);
	}
}

void ADeliveryZone::OnZoneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
								  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
								  bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;

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
	bool bIsCorrectZone = false;

	// 매칭해둔 상자의 GameplayTag와 이 구역의 ZoneType을 비교 검사합니다.
	FGameplayTag BoxType = Data.BoxTypeTag;
	
	if (ZoneType == EDestinationType::ZoneA && BoxType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Box.Type.ZoneA")))) bIsCorrectZone = true;
	else if (ZoneType == EDestinationType::ZoneB && BoxType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Box.Type.ZoneB")))) bIsCorrectZone = true;
	else if (ZoneType == EDestinationType::ZoneC && BoxType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Box.Type.ZoneC")))) bIsCorrectZone = true;
	else if (ZoneType == EDestinationType::Emergency && BoxType.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Box.Type.Emergency")))) bIsCorrectZone = true;

	int32 ScoreChange = bIsCorrectZone ? Data.BaseScore : Data.DamagePenalty;

	if (bIsCorrectZone)
	{
		UE_LOG(LogDelivery, Log, TEXT("[Server] Success: Delivered to the correct zone! Score +%d"), ScoreChange);
		Box->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Delivered")));
	}
	else
	{
		UE_LOG(LogDelivery, Warning, TEXT("[Server] Fail: Delivered to the wrong zone! Penalty %d"), ScoreChange);
		Box->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Failed")));
	}

	// 1. 팀 점수 반영 (AParcelGameState -> UTeamScoreComponent)
	if (AParcelGameState* GameState = GetWorld()->GetGameState<AParcelGameState>())
	{
		if (UTeamScoreComponent* TeamScoreComp = GameState->GetTeamScoreComponent())
		{
			TeamScoreComp->AddTeamScore(ScoreChange);
		}
	}

	// 2. 개인 플레이어 점수 및 콤보 반영 (AParcelPlayerState -> UPlayerStatComponent)
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

	// 사용 완료된 상자는 디스폰 서브시스템을 통해 깔끔하게 삭제
	if (UDeliverySubsystem* DeliverySubsystem = GetWorld()->GetSubsystem<UDeliverySubsystem>())
	{
		DeliverySubsystem->DespawnBox(Box);
	}
}