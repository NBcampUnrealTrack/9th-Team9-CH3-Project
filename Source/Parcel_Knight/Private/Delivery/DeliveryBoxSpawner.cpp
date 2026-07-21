#include "Delivery/DeliveryBoxSpawner.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Delivery/DeliverySubsystem.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Kismet/GameplayStatics.h"

ADeliveryBoxSpawner::ADeliveryBoxSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComp);

	SpawnerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpawnerMesh"));
	SpawnerMesh->SetupAttachment(RootComp);

	// 스포너 기본 메시 로드
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultSpawnerMesh(TEXT("/Script/Engine.StaticMesh'/Game/Delivery/Meshes/SM_AssemblyLineBox01.SM_AssemblyLineBox01'"));
	if (DefaultSpawnerMesh.Succeeded())
	{
		SpawnerMesh->SetStaticMesh(DefaultSpawnerMesh.Object);
	}

	ConveyorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConveyorMesh"));
	ConveyorMesh->SetupAttachment(RootComp);

	ConveyorVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ConveyorVolume"));
	ConveyorVolume->SetupAttachment(RootComp);
	ConveyorVolume->SetCollisionProfileName(TEXT("Trigger"));
	ConveyorVolume->SetBoxExtent(FVector(100.f, 40.f, 20.f)); // 컨베이어 물리 체크 기본 크기
	ConveyorVolume->SetRelativeLocation(FVector(150.f, 0.f, 10.f)); // 앞쪽 컨베이어 위치에 맞춰 오프셋

	SpawnBoundsVisualizer = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBoundsVisualizer"));
	SpawnBoundsVisualizer->SetupAttachment(RootComp);
	SpawnBoundsVisualizer->SetBoxExtent(FVector(30.f, 30.f, 30.f));
	SpawnBoundsVisualizer->SetCollisionProfileName(TEXT("NoCollision"));
	SpawnBoundsVisualizer->bHiddenInGame = true;
	SpawnBoundsVisualizer->SetRelativeLocation(FVector(0.f, 0.f, 50.f)); // 스포너 기계 내부 스폰 지점
	
	ForwardArrowVisualizer = CreateDefaultSubobject<UArrowComponent>(TEXT("ForwardArrowVisualizer"));
	ForwardArrowVisualizer->SetupAttachment(RootComp);
	ForwardArrowVisualizer->ArrowColor = FColor::Cyan;
	ForwardArrowVisualizer->ArrowSize = 1.0f;
	ForwardArrowVisualizer->bHiddenInGame = true;

	// 컨베이어 벨트 구동 소리 오디오 컴포넌트 초기화
	ConveyorSoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ConveyorSoundComponent"));
	ConveyorSoundComponent->SetupAttachment(RootComp);
	ConveyorSoundComponent->bAutoActivate = false;

	// 컨베이어 기본 3D 소리 감쇄 에셋 (ATT_Conveyor) 경로 자동 연결
	static ConstructorHelpers::FObjectFinder<USoundAttenuation> DefaultConveyorAttenuation(TEXT("/Script/Engine.SoundAttenuation'/Game/Delivery/sounds/ATT_Conveyor.ATT_Conveyor'"));
	if (DefaultConveyorAttenuation.Succeeded())
	{
		ConveyorSoundAttenuation = DefaultConveyorAttenuation.Object;
	}
}

void ADeliveryBoxSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 서버 권한이 있을 때만 타이머를 가동하여 상자를 주기적으로 스폰합니다.
	if (HasAuthority())
	{
		ActivateSpawner(SpawnInterval);
	}

	// 로컬 클라이언트 및 서버(리스너)에서 컨베이어 루핑 소리가 흘러나오도록 3D 재생 설정
	if (ConveyorSoundComponent && ConveyorSoundAsset)
	{
		ConveyorSoundComponent->SetSound(ConveyorSoundAsset);
		if (ConveyorSoundAttenuation)
		{
			ConveyorSoundComponent->AttenuationSettings = ConveyorSoundAttenuation;
		}
		ConveyorSoundComponent->Play();
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

void ADeliveryBoxSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 서버에서 컨베이어 오버랩 물리 밀어주기 처리
	if (HasAuthority() && bEnableConveyor && ConveyorVolume)
	{
		TArray<UPrimitiveComponent*> OverlappingComponents;
		ConveyorVolume->GetOverlappingComponents(OverlappingComponents);

		for (UPrimitiveComponent* Comp : OverlappingComponents)
		{
			// 물리가 켜진 액터 컴포넌트(상자)만 타겟으로 함
			if (Comp && Comp->IsSimulatingPhysics())
			{
				FVector ForwardDir = -GetActorForwardVector(); // 180도 반대 방향 (뒤쪽)으로 밀어줌
				FVector TargetVelocity = ForwardDir * ConveyorSpeed;
				FVector CurrentVelocity = Comp->GetPhysicsLinearVelocity();

				// 중력 방향(Z축) 속도는 유지하고 X, Y 평면 속도만 밀어줌
				FVector NewVelocity = FVector(TargetVelocity.X, TargetVelocity.Y, CurrentVelocity.Z);
				Comp->SetPhysicsLinearVelocity(NewVelocity);
			}
		}
	}
}

void ADeliveryBoxSpawner::TriggerRandomSpawn()
{
	if (!GetWorld()) return;

	// Spawn Random Box 함수를 서브시스템을 통해 호출
	UDeliverySubsystem* DeliverySubsystem = GetWorld()->GetSubsystem<UDeliverySubsystem>();
	if (DeliverySubsystem && SpawnBoundsVisualizer)
	{
		// 스포너 머리/내부에 있는 SpawnBoundsVisualizer의 실시간 좌표에서 소환
		DeliverySubsystem->SpawnRandomBox(SpawnBoundsVisualizer->GetComponentLocation(), SpawnBoundsVisualizer->GetComponentRotation());

		// 모든 유저 화면에서 소환 소리가 나도록 멀티캐스트 재생
		Multicast_PlaySpawnSound();
	}
}

void ADeliveryBoxSpawner::Multicast_PlaySpawnSound_Implementation()
{
	if (BoxSpawnSound && SpawnBoundsVisualizer)
	{
		// 상자 생성 지점(기계 안쪽)에서 지정한 감쇄 에셋(ATT_Conveyor)을 사용하여 재생
		UGameplayStatics::PlaySoundAtLocation(
			this, 
			BoxSpawnSound, 
			SpawnBoundsVisualizer->GetComponentLocation(), 
			FRotator::ZeroRotator, 
			1.0f, 
			1.0f, 
			0.0f, 
			ConveyorSoundAttenuation
		);
	}
}