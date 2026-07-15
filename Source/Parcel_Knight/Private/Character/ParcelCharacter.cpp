#include "Character/ParcelCharacter.h"
#include "ParcelLog.h"
#include "Character/RagdollComponent.h"
#include "Character/ParcelHeroComponent.h"
#include "Character/ParcelInteractionComponent.h"
#include "Character/ParcelMovementStatComponent.h"
#include "Character/CharacterCarryComponent.h"
#include "Character/ParcelPlayerStateComponent.h"
#include "Components/DFKnockbackComponent.h"
#include "Components/DFStatusEffectComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Core/HealthComponent.h"
#include "Core/ParcelPlayerState.h"


DEFINE_LOG_CATEGORY(LogCharacter);

AParcelCharacter::AParcelCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    SetReplicateMovement(true);
    
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
  PlayerStateComp = CreateDefaultSubobject<UParcelPlayerStateComponent>(TEXT("PlayerStateComp"));
	RagdollComp = CreateDefaultSubobject<URagdollComponent>(TEXT("RagdollComp"));
	StatusEffectComponent = CreateDefaultSubobject<UDFStatusEffectComponent>(TEXT("StatusEffectComponent"));
	KnockbackComponent = CreateDefaultSubobject<UDFKnockbackComponent>(TEXT("KnockbackComponent"));
	HeroComp = CreateDefaultSubobject<UParcelHeroComponent>(TEXT("HeroComp"));
	InteractionComp = CreateDefaultSubobject<UParcelInteractionComponent>(TEXT("InteractionComp"));
	MovementStatComp = CreateDefaultSubobject<UParcelMovementStatComponent>(TEXT("MovementStatComp"));
	CarryComp = CreateDefaultSubobject<UCharacterCarryComponent>(TEXT("CarryComp"));

	bIsRagdoll = false;
	bIsGettingUp = false;
}

void AParcelCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AParcelCharacter::OnJumped_Implementation()
{
    Super::OnJumped_Implementation();
    // 공중에 뜨는 물리적 타이밍에 InAir 태그 주입
    if (HasAuthority() && PlayerStateComp)
    {
        PlayerStateComp->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.InAir")));
    }
}

void AParcelCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    // 바닥 지면에 닿는 물리적 타이밍에 InAir 태그를 제거하고 무브먼트 동기화 리프레시
    if (HasAuthority() && PlayerStateComp)
    {
        PlayerStateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.InAir")));
    }
    if (MovementStatComp)
    {
        MovementStatComp->RefreshMoveSpeed();
    }
}

void AParcelCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // 부모 클래스의 입력 설정을 먼저 실행
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    
    // 입력 컴포넌트 통로를 히어로 컴포넌트 내부의 바인딩 연산으로 넘김
    if (HeroComp)
    {
       HeroComp->InitializePlayerInput(PlayerInputComponent);
    }
}

void AParcelCharacter::SetRagdollState(bool bNewIsRagdoll, bool bNewIsGettingUp)
{
	bIsRagdoll = bNewIsRagdoll;
	bIsGettingUp = bNewIsGettingUp;

	if (bIsGettingUp)
	{
		GetWorldTimerManager().ClearTimer(GetUpTimerHandle);

		GetWorldTimerManager().SetTimer(
			GetUpTimerHandle,
			this,
			&AParcelCharacter::FinishGetUp,
			1.3f,
			false
		);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(GetUpTimerHandle);
	}
}

void AParcelCharacter::FinishGetUp()
{
	bIsGettingUp = false;
}

void AParcelCharacter::PossessedBy(AController* NewController)
{
    // 서버의 possession 처리 흐름을 유지
    Super::PossessedBy(NewController);

    // 서버 Possessed 시점에 HeroComponent에 이벤트를 넘김
    if (HeroComp)
    {
       HeroComp->AddInputMappingContext();
    }

	//HealthComponent를 찾아서 사망을 바인드하는 코드, TODO: HealthCompoent확인 필요
    if (UHealthComponent* HealthComp = FindComponentByClass<UHealthComponent>())
    {
        if (AParcelPlayerState* PS = GetPlayerState<AParcelPlayerState>())
        {
            HealthComp->OnDeathDelegate.AddUniqueDynamic(PS, &AParcelPlayerState::HandleDeath);
        }
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

void AParcelCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}