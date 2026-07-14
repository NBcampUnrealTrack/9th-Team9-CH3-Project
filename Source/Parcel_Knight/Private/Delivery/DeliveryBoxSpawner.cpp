#include "Delivery/DeliveryBoxSpawner.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "TimerManager.h"
#include "Delivery/DeliverySubsystem.h"

ADeliveryBoxSpawner::ADeliveryBoxSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComp);
	
	SpawnBoundsVisualizer = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBoundsVisualizer"));
	SpawnBoundsVisualizer->SetupAttachment(RootComp);
	SpawnBoundsVisualizer->SetBoxExtent(FVector(50.f, 50.f, 50.f));
	SpawnBoundsVisualizer->SetCollisionProfileName(TEXT("NoCollision"));
	SpawnBoundsVisualizer->bHiddenInGame = true;
	
	ForwardArrowVisualizer = CreateDefaultSubobject<UArrowComponent>(TEXT("ForwardArrowVisualizer"));
	ForwardArrowVisualizer->SetupAttachment(RootComp);
	ForwardArrowVisualizer->ArrowColor = FColor::Cyan;
	ForwardArrowVisualizer->ArrowSize = 1.0f;
	ForwardArrowVisualizer->bHiddenInGame = true;
}

void ADeliveryBoxSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 서버 권한이 있을 때만 타이머를 가동하여 상자를 주기적으로 스폰합니다.
	if (HasAuthority())
	{
		ActivateSpawner(SpawnInterval);
	}
}

void ADeliveryBoxSpawner::ActivateSpawner(float InInterval)
{
	// 리슨 서버에서만 스폰 타이머가 작동
	if (!HasAuthority() || !GetWorld()) return;
	
	// 기존 타이머 안전하게 청소
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);

	// C++ 엔진 시계에 타이머 매핑
	GetWorld()->GetTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&ADeliveryBoxSpawner::TriggerRandomSpawn,
		InInterval,
		true
	);
}

void ADeliveryBoxSpawner::TriggerRandomSpawn()
{
	if (!GetWorld()) return;

	// Spawn Random Box 함수를 서브시스템을 통해 호출
	UDeliverySubsystem* DeliverySubsystem = GetWorld()->GetSubsystem<UDeliverySubsystem>();
	if (DeliverySubsystem)
	{
		DeliverySubsystem->SpawnRandomBox(GetActorLocation(), GetActorRotation());
	}
}