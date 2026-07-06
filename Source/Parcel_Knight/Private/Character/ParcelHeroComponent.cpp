#include "Character/ParcelHeroComponent.h"
#include "ParcelLog.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Character/ParcelCharacter.h"
#include "Character/RagdollComponent.h"
#include "Character/ParcelInteractionComponent.h"
#include "Character/ParcelMovementStatComponent.h"
#include "Character/CharacterCarryComponent.h"
#include "Character/ParcelPlayerStateComponent.h"

DEFINE_LOG_CATEGORY(LogHeroComp);

UParcelHeroComponent::UParcelHeroComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    
    // [Server] : 컴포넌트에서 Server RPC 가동
    SetIsReplicatedByDefault(true);

    // 카메라
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->TargetArmLength = 350.f;
    SpringArm->bDoCollisionTest = true;
    SpringArm->ProbeChannel = ECC_Camera;
    SpringArm->ProbeSize = CameraCollisionProbeSize;
    SpringArm->bUsePawnControlRotation = true;
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 10.f;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->bUsePawnControlRotation = false;
}

void UParcelHeroComponent::BeginPlay()
{
    Super::BeginPlay();

    // 부모 캐릭터 루트에 카메라 부착
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
       SpringArm->AttachToComponent(Character->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
       FollowCamera->AttachToComponent(SpringArm, FAttachmentTransformRules::SnapToTargetNotIncludingScale, USpringArmComponent::SocketName);
    
       HEROCOMP_LOG(Log, TEXT("[%s] 캐릭터에 카메라 컴포넌트 부착 완료."), *Character->GetName());
    }

    AddInputMappingContext();
}

void UParcelHeroComponent::ResetCameraAttachment()
{
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
       if (SpringArm)
       {
          SpringArm->AttachToComponent(Character->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
          SpringArm->SetRelativeLocation(FVector::ZeroVector);
       }
    }
}

void UParcelHeroComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    URagdollComponent* RagdollComp = Character->FindComponentByClass<URagdollComponent>();

    // 래그돌 상태가 활성화 되었을 때만 카메라 연산 가동함(카메라 보정)
    if (Character->IsLocallyControlled() && RagdollComp && RagdollComp->IsRagdoll() && Character->GetMesh() && SpringArm)
    {
       const FVector HeadLocation = Character->GetMesh()->GetSocketLocation(TEXT("head"));
       const FRotator ViewRotation = Character->GetController() ? Character->GetController()->GetControlRotation() : Character->GetActorRotation();
       const FVector CameraBackDirection = -FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::X);
       const FVector TargetLocation = HeadLocation + FVector::UpVector * RagdollCameraHeightOffset + CameraBackDirection * RagdollCameraBackOffset;

       SpringArm->SetWorldLocation(TargetLocation);
    }
}

void UParcelHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
    AddInputMappingContext();

    if (!CanProcessLocalInput()) return;

    // IA 바인딩
    UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EnhancedInputComponent) return;
    
    if (MoveAction) EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UParcelHeroComponent::Move);
    
    if (LookAction) EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &UParcelHeroComponent::Look);
    
    if (JumpAction)
    {
       EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &UParcelHeroComponent::StartJump);
       EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &UParcelHeroComponent::StopJump);
    }
    
    if (SprintAction)
    {
       EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &UParcelHeroComponent::StartSprint);
       EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &UParcelHeroComponent::StopSprint);
       EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &UParcelHeroComponent::StopSprint);
    }
    
    if (RagdollAction) EnhancedInputComponent->BindAction(RagdollAction, ETriggerEvent::Started, this, &UParcelHeroComponent::TestRagdoll);
    
    if (InteractAction) 
    {
       EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &UParcelHeroComponent::Interact);
    }
    
    HEROCOMP_LOG(Log, TEXT("Enhanced Input 바인딩 완료."));
}

void UParcelHeroComponent::Move(const FInputActionValue& Value)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!CanProcessLocalInput() || !Character || !Character->GetController()) return;

    // [Add] 방어 코드 : 래그돌 상태에서는 입력 처리 불가
    URagdollComponent* RagdollComp = Character->FindComponentByClass<URagdollComponent>();
    if (RagdollComp && RagdollComp->IsRagdoll()) return;
    
    const FVector2D MoveValue = Value.Get<FVector2D>();
    const FRotator ControlRotation = Character->GetController()->GetControlRotation();
    const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    Character->AddMovementInput(ForwardDirection, MoveValue.Y);
    Character->AddMovementInput(RightDirection, MoveValue.X);
}

void UParcelHeroComponent::Look(const FInputActionValue& Value)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!CanProcessLocalInput() || !Character) return;

    const FVector2D LookValue = Value.Get<FVector2D>();
    Character->AddControllerYawInput(LookValue.X);
    Character->AddControllerPitchInput(LookValue.Y);
}

void UParcelHeroComponent::StartJump(const FInputActionValue& Value)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!CanProcessLocalInput() || !Character) return;

    // Throwing 액션 중에는 물리적인 점프 발동을 차단
    if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(Character))
    {
        if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
        {
            if (StateComp->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.Action.Throwing")))) return;
        }
    }

    Character->Jump();
}

void UParcelHeroComponent::StopJump(const FInputActionValue& Value)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (CanProcessLocalInput() && Character) Character->StopJumping();
}

void UParcelHeroComponent::StartSprint(const FInputActionValue& Value)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    URagdollComponent* RagdollComp = Character->FindComponentByClass<URagdollComponent>();
    if (!CanProcessLocalInput() || (RagdollComp && RagdollComp->IsRagdoll())) return;

    ApplySprintSpeed(true);
    if (!Character->HasAuthority()) ServerSetSprinting(true);
}

void UParcelHeroComponent::StopSprint(const FInputActionValue& Value)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!CanProcessLocalInput() || !Character) return;

    ApplySprintSpeed(false);
    if (!Character->HasAuthority()) ServerSetSprinting(false);
}

void UParcelHeroComponent::TestRagdoll(const FInputActionValue& Value)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!CanProcessLocalInput() || !Character) return;

    URagdollComponent* RagdollComp = Character->FindComponentByClass<URagdollComponent>();
    if (!RagdollComp) return;

    const bool bWasRagdoll = RagdollComp->IsRagdoll();
	if (bWasRagdoll && !RagdollComp->IsRagdollCloseToGround()) 
	{
		HEROCOMP_LOG(Warning, TEXT("래그돌 해제 실패 : 현재 공중에 떠 있는 상태입니다. (지면과 너무 멂)"));
		return;
	}

    RagdollComp->ToggleRagdoll();

    // 래그돌이 켜질 때만 컴포넌트 틱을 킴
    if (RagdollComp->IsRagdoll())
    {
       HEROCOMP_LOG(Log, TEXT("래그돌 상태 진입: 카메라 보정을 위한 컴포넌트 틱 활성화"));
       PrimaryComponentTick.SetTickFunctionEnable(true);
    }
    else
    {
       HEROCOMP_LOG(Log, TEXT("래그돌 상태 해제: 카메라 위치 복구 및 컴포넌트 틱 비활성화"));
       PrimaryComponentTick.SetTickFunctionEnable(false);
       if (SpringArm)
       {
          SpringArm->AttachToComponent(Character->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
          SpringArm->SetRelativeLocation(FVector::ZeroVector);
       }
    }
}

void UParcelHeroComponent::Interact(const FInputActionValue& Value)
{
    if (!CanProcessLocalInput()) return;
    
    HEROCOMP_LOG(Log, TEXT("상호작용 조작(E키) 감지: InteractionComponent 호출"));
    
    // [Add] 방어 코드 : 캐릭터가 없으면 상호작용 중단
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;
    
    // [Add] 방어 코드 : 래그돌 도중에는 상호작용 불가
    URagdollComponent* RagdollComp = Character->FindComponentByClass<URagdollComponent>();
    if (RagdollComp && RagdollComp->IsRagdoll()) return;

    // InAir 상태에서는 집기 상호작용 불가
    if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(Character))
    {
        if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
        {
            if (StateComp->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.InAir")))) return;
        }
    }
    
    // 장착된 InteractionComponent 호출
    if (AParcelCharacter* OwnerChar = Cast<AParcelCharacter>(GetOwner()))
    {
       if (UParcelInteractionComponent* InteractComp = OwnerChar->GetParcelInteractionComponent())
       {
          InteractComp->PrimaryInteract();
       }
    }
}


void UParcelHeroComponent::ServerSetSprinting_Implementation(bool bNewIsSprinting)
{
    HEROCOMP_LOG(Log, TEXT("[Server] 클라이언트의 요청으로 달리기 상태 변경 적용: %s"), bNewIsSprinting ? TEXT("True") : TEXT("False"));
    ApplySprintSpeed(bNewIsSprinting);
}

void UParcelHeroComponent::ApplySprintSpeed(bool bNewIsSprinting)
{
    AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(GetOwner());
    if (!ParcelChar) return;

    // 서버 전용 권한 확인 후 중앙 상태 창고 컴포넌트에 실시간 달리기 태그 토글 제어
    if (ParcelChar->HasAuthority())
    {
       if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
       {
          FGameplayTag SprintTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Sprinting"));
          if (bNewIsSprinting) StateComp->AddStateTag(SprintTag);
          else StateComp->RemoveStateTag(SprintTag);
       }
    }

    // MovementStat을 담당하는 매니저에 속도 계산 위임
    if (UParcelMovementStatComponent* StatComp = ParcelChar->GetParcelMovementStatComponent())
    {
       StatComp->RefreshMoveSpeed();
    }
}

void UParcelHeroComponent::AddInputMappingContext()
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character || !Character->IsLocallyControlled()) return;

    APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
    if (!PlayerController || !PlayerController->GetLocalPlayer()) return;

    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
    if (!Subsystem || !InputMappingContext) return;

    Subsystem->RemoveMappingContext(InputMappingContext);
    Subsystem->AddMappingContext(InputMappingContext, 0);
}

bool UParcelHeroComponent::CanProcessLocalInput() const
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    return Character && Character->GetController() && Character->IsLocallyControlled();
}

void UParcelHeroComponent::EnterRagdollCameraMode()
{
	PrimaryComponentTick.SetTickFunctionEnable(true);
	HEROCOMP_LOG(Log, TEXT("카메라 래그돌 모드 진입"));
}

void UParcelHeroComponent::ExitRagdollCameraMode()
{
	PrimaryComponentTick.SetTickFunctionEnable(false);
	ResetCameraAttachment();
	HEROCOMP_LOG(Log, TEXT("카메라 래그돌 모드 해제"));
}