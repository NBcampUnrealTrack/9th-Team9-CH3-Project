#include "Character/ParcelCharacter.h"
#include "ParcelLog.h"
#include "Character/RagdollComponent.h"
#include "Character/ParcelHeroComponent.h"
#include "Character/ParcelInteractionComponent.h"
#include "Character/ParcelMovementStatComponent.h"
#include "Character/CharacterCarryComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"


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

	// 컴포넌트 조립
	RagdollComp = CreateDefaultSubobject<URagdollComponent>(TEXT("RagdollComp"));
	HeroComp = CreateDefaultSubobject<UParcelHeroComponent>(TEXT("HeroComp"));
	InteractionComp = CreateDefaultSubobject<UParcelInteractionComponent>(TEXT("InteractionComp"));
	MovementStatComp = CreateDefaultSubobject<UParcelMovementStatComponent>(TEXT("MovementStatComp"));
	CarryComp = CreateDefaultSubobject<UCharacterCarryComponent>(TEXT("CarryComp"));
}

void AParcelCharacter::BeginPlay()
{
	// 위치, 회전 같은 Actor Movement를 서버에서 클라이언트로 복제
	// ACharacter의 기본 CharacterMovement 복제와 함께 동작합니다.
	SetReplicateMovement(true);
	
	Super::BeginPlay();
}

void AParcelCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 캐릭터의 상태 태그 컨테이너를 모든 클라이언트에게 동기화
	DOREPLIFETIME(AParcelCharacter, CharacterStateTags);
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

void AParcelCharacter::AddStateTag(FGameplayTag NewStateTag)
{
	if (!HasAuthority() || !NewStateTag.IsValid()) return;

	if (!CharacterStateTags.HasTagExact(NewStateTag))
	{
		CharacterStateTags.AddTag(NewStateTag);
        
		PLAYER_LOG(All, TEXT("[Server] 캐릭터[%s] 상태 태그 추가됨: %s"), *GetName(), *NewStateTag.ToString());

		// 호스트 유저의 화면 연출을 위해 OnRep 수동 강제 트리거
		OnRep_CharacterStateTags();
	}
}

void AParcelCharacter::RemoveStateTag(FGameplayTag StateTag)
{
	if (!HasAuthority() || !StateTag.IsValid()) return;

	if (CharacterStateTags.HasTagExact(StateTag))
	{
		CharacterStateTags.RemoveTag(StateTag);
        
		PLAYER_LOG(All, TEXT("[Server] 캐릭터[%s] 상태 태그 제거됨: %s"), *GetName(), *StateTag.ToString());

		// 호스트 유저의 화면 연출을 위해 OnRep 수동 강제 트리거
		OnRep_CharacterStateTags();
	}
}

void AParcelCharacter::OnRep_CharacterStateTags()
{
	// Todo : [Client] ABP에 신호를 주거나, 특정 이펙트/사운드를 켜고 끄는 연출을 처리
	PLAYER_LOG(All, TEXT("[Client] 캐릭터[%s] 상태 태그 컨테이너 동기화됨. 현재 태그 목록: %s"), *GetName(), *CharacterStateTags.ToString());
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
	