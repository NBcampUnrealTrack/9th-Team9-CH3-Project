#include "Character/ParcelCharacter.h"
#include "ParcelLog.h"
#include "Character/RagdollComponent.h"
#include "Character/ParcelHeroComponent.h"
#include "Character/ParcelInteractionComponent.h"
#include "Character/ParcelMovementStatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogCharacter);

AParcelCharacter::AParcelCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// 캐릭터 액터 자체를 네트워크에 복제
	bReplicates = true;

	// 움직임이 잦은 플레이어 캐릭터라서 기본보다 높은 빈도로 네트워크 갱신을 요청
	SetNetUpdateFrequency(100.f);

	// 네트워크 상태가 안정적일 때도 너무 낮은 빈도로 떨어지지 않게 최소 갱신 빈도를 지정합니다.
	SetMinNetUpdateFrequency(33.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 이동 입력 방향을 바라보도록 캐릭터를 자동 회전
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// 공중에서 이동 입력이 얼마나 반영되는지 정합니다.
	GetCharacterMovement()->AirControl = 0.35f;

	// 래그돌 상태 전환과 복제를 담당하는 컴포넌트를 생성
	RagdollComp = CreateDefaultSubobject<URagdollComponent>(TEXT("RagdollComp"));
	
	// 조작 및 카메라를 담당하는 컴포넌트 생성
	HeroComp = CreateDefaultSubobject<UParcelHeroComponent>(TEXT("HeroComp"));
	
	// 상호작용 컴포넌트 생성
	InteractionComp = CreateDefaultSubobject<UParcelInteractionComponent>(TEXT("InteractionComp"));
	
	// 이동 관련 스탯 컴포넌트 인스턴스 생성 및 부착
	MovementStatComp = CreateDefaultSubobject<UParcelMovementStatComponent>(TEXT("MovementStatComp"));
}

void AParcelCharacter::BeginPlay()
{
	// 위치, 회전 같은 Actor Movement를 서버에서 클라이언트로 복제
	// ACharacter의 기본 CharacterMovement 복제와 함께 동작합니다.
	SetReplicateMovement(true);
	
	Super::BeginPlay();
}

void AParcelCharacter::PossessedBy(AController* NewController)
{
	// 서버의 possession 처리 흐름을 유지합니다.
	Super::PossessedBy(NewController);

	// 서버 Possessed 시점에 HeroComponent에 이벤트를 넘김
	if (HeroComp)
	{
		HeroComp->AddInputMappingContext();
	}
}

void AParcelCharacter::OnRep_Controller()
{
	// 클라이언트에서 복제된 Controller 변경 처리를 부모 클래스에 맡깁니다.
	Super::OnRep_Controller();

	// 클라이언트의 Controller 복제 시점에 HeroComponent에 이벤트를 넘김
	if (HeroComp)
	{
		HeroComp->AddInputMappingContext();
	}
}


void AParcelCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 부모 클래스의 입력 설정을 먼저 실행합니다.
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// 입력 컴포넌트 통로를 히어로 컴포넌트 내부의 바인딩 연산으로 넘김
	if (HeroComp)
	{
		HeroComp->InitializePlayerInput(PlayerInputComponent);
	}
}
	