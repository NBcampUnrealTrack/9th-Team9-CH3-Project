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
#include "GameFramework/Character.h"
#include "Character/CharacterCarryComponent.h"

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

	// 1) 상자 자체가 직접 닿았을 때 (예: 바닥에 굴리거나 던져진 상태로 도달)
	if (ADeliveryBox* Box = Cast<ADeliveryBox>(OtherActor))
	{
		if (Box->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Delivered")))) return;

		ProcessDelivery(Box);
	}
	// 2) 캐릭터가 상자를 들고 들어왔을 때 (상자 콜리전이 NoCollision 상태이므로 캐릭터가 닿은 것으로 감지)
	else if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		if (UCharacterCarryComponent* CarryComponent = Character->FindComponentByClass<UCharacterCarryComponent>())
		{
			if (CarryComponent->IsCarrying())
			{
				ADeliveryBox* BoxToDeliver = CarryComponent->GetCarriedBox();
				if (BoxToDeliver && !BoxToDeliver->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Delivered"))))
				{
					// 정상적으로 들고 있던 상자를 놓게(소유 및 속도 복원) 처리한 후 배송을 진행합니다.
					CarryComponent->Drop();
					ProcessDelivery(BoxToDeliver);
				}
			}
		}
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
		Box->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Delivered")));
	}
	else
	{
		Box->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Failed")));
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

	// 1) 개인 점수 가산
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

	// 2) 팀 점수 가산 및 현재 총점 획득
	int32 CurrentTeamScore = 0;
	if (AParcelGameState* GameState = GetWorld()->GetGameState<AParcelGameState>())
	{
		if (UTeamScoreComponent* TeamScoreComp = GameState->GetTeamScoreComponent())
		{
			TeamScoreComp->AddTeamScore(ScoreChange);
			CurrentTeamScore = TeamScoreComp->GetTeamScore();
		}
	}

	// 3) 화면 디버그 메시지 및 콘솔 로그 출력
	FString DeliveryPlayerName = PlayerState ? PlayerState->GetPlayerName() : TEXT("Unknown Player");
	FString DeliveryResultStr = bIsCorrectZone ? TEXT("배송 성공") : TEXT("오배송");
	
	// 콘솔 로그 출력
	DELIVERYZONE_LOG(Log, TEXT("[Server] %s - 플레이어: %s, 점수변동: %+d, 현재 팀 점수: %d"), 
		*DeliveryResultStr, *DeliveryPlayerName, ScoreChange, CurrentTeamScore);

	// 게임 화면 좌측 상단에 실시간 피드백 띄우기
	if (GEngine)
	{
		FColor TextColor = bIsCorrectZone ? FColor::Green : FColor::Red;
		FString DebugMessage = FString::Printf(TEXT("[%s] %s 님이 상자를 배송했습니다! (%+d점 / 팀 점수: %d)"),
			*DeliveryResultStr, *DeliveryPlayerName, ScoreChange, CurrentTeamScore);
		
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, TextColor, DebugMessage);
	}
	
	// 사용 완료된 상자는 서브시스템을 통해 삭제
	if (UDeliverySubsystem* DeliverySubsystem = GetWorld()->GetSubsystem<UDeliverySubsystem>())
	{
		DeliverySubsystem->DespawnBox(Box);
	}
}