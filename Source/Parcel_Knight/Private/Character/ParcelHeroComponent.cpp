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
#include "Delivery/DeliveryBox.h"
#include "UI/ParcelInGameESCMenuWidget.h"
#include "Components/DFStatusEffectComponent.h"
#include "Core/HealthComponent.h"
#include "Core/ParcelPlayerController.h"
#include "UI/ParcelLobbyHUDWidget.h"
#include "Core/ParcelGameUserSettings.h"

DEFINE_LOG_CATEGORY(LogHeroComp);

UParcelHeroComponent::UParcelHeroComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);
    MouseSensitivity = 1.0f;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->TargetArmLength = 0.f;
    SpringArm->bDoCollisionTest = true;
    SpringArm->ProbeChannel = ECC_Camera;
    SpringArm->ProbeSize = CameraCollisionProbeSize;
    SpringArm->bUsePawnControlRotation = true;
    SpringArm->bEnableCameraLag = false;

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
       SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
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
          SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
       }
    }
}

void UParcelHeroComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    if (Character->IsLocallyControlled() && bIsChargingThrow)
    {
        // [UI] 방어 코드 : 던지는 도중 맞거나 래그돌 등 상자 놓친 경우 예외 처리
        UCharacterCarryComponent* CarryComp = Character->FindComponentByClass<UCharacterCarryComponent>();
        if (!CarryComp || !CarryComp->IsCarrying())
        {
            HEROCOMP_LOG(Warning, TEXT("차징 연출을 강제 취소합니다."));
            bIsChargingThrow = false;
            CurrentThrowChargeTime = 0.f;
            
            OnThrowChargeChanged.Broadcast(false, 0.0f);
            
            if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(Character))
            {
                if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
                {
                    StateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.Action.Throwing")));
                }
            }
            
            URagdollComponent* RagdollComp = Character->FindComponentByClass<URagdollComponent>();
            bool bNeedsTick = RagdollComp && RagdollComp->IsRagdoll();
            if (!bNeedsTick)
            {
                PrimaryComponentTick.SetTickFunctionEnable(false);
            }
            return;
        }
        
        CurrentThrowChargeTime += DeltaTime;
        if (CurrentThrowChargeTime > MaxThrowChargeTime)
        {
            CurrentThrowChargeTime = MaxThrowChargeTime;
        }
        
        // [UI] 프레임마다 변경되는 ChargeRatio 전달
        float ChargeRatio = MaxThrowChargeTime > 0.f ? FMath::Clamp(CurrentThrowChargeTime / MaxThrowChargeTime, 0.f, 1.f) : 0.f;
        OnThrowChargeChanged.Broadcast(true, ChargeRatio);
        
    }

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

	  if (const UParcelGameUserSettings* Settings = UParcelGameUserSettings::GetParcelGameUserSettings())
	  {
		  SetMouseSensitivity(Settings->GetMouseSensitivity());
	  }

    // IA 바인딩 (SetupPlayerInputComponent가 재호출돼도 한 번만 등록)
    if (bInputBound) return;

    UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EnhancedInputComponent) return;

    bInputBound = true;

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
    if (InteractAction) EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &UParcelHeroComponent::Interact);
    
    if (ThrowAction)
    {
       EnhancedInputComponent->BindAction(ThrowAction, ETriggerEvent::Started, this, &UParcelHeroComponent::StartThrow);
       EnhancedInputComponent->BindAction(ThrowAction, ETriggerEvent::Completed, this, &UParcelHeroComponent::ReleaseThrow);
    }
    
    if (InGameMenuAction) EnhancedInputComponent->BindAction(InGameMenuAction, ETriggerEvent::Started, this, &UParcelHeroComponent::ToggleInGameMenu);
    if (OpenChatAction) EnhancedInputComponent->BindAction(OpenChatAction, ETriggerEvent::Started, this, &UParcelHeroComponent::Input_OpenChat);
    
    if (LobbyMenuAction)
    {
        // [인풋 필터링] 무한 연사 토글 버그를 예방하기 위해 Triggered 대신 단 한 번 발동하는 Started로 엄격한 격상 완료!
        EnhancedInputComponent->BindAction(LobbyMenuAction, ETriggerEvent::Started, this, &UParcelHeroComponent::Input_ToggleLobbyMenu);
    }

    if (UseSlotAction1) EnhancedInputComponent->BindAction(UseSlotAction1, ETriggerEvent::Started, this, &UParcelHeroComponent::UseSlot1);
    if (UseSlotAction2) EnhancedInputComponent->BindAction(UseSlotAction2, ETriggerEvent::Started, this, &UParcelHeroComponent::UseSlot2);
    if (UseSlotAction3) EnhancedInputComponent->BindAction(UseSlotAction3, ETriggerEvent::Started, this, &UParcelHeroComponent::UseSlot3);

    HEROCOMP_LOG(Log, TEXT("Enhanced Input 바인딩 완료."));
}

void UParcelHeroComponent::Move(const FInputActionValue& Value)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!CanProcessLocalInput() || !Character || !Character->GetController()) return;

    // [Add] 방어 코드 : 래그돌 상태에서는 입력 처리 불가
    URagdollComponent* RagdollComp = Character->FindComponentByClass<URagdollComponent>();
    if (RagdollComp && RagdollComp->IsRagdoll()) return;
    
	// 반전 함정
	FVector2D MoveValue = Value.Get<FVector2D>();
	if (const UDFStatusEffectComponent* StatusEffectComponent = Character->FindComponentByClass<UDFStatusEffectComponent>())
	{
		if (StatusEffectComponent->IsInputInverted())
		{
			MoveValue *= -1.0f;
		}
	}

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

	const FVector2D RawLookValue = Value.Get<FVector2D>();
	const FVector2D LookValue = RawLookValue * MouseSensitivity;
    
    Character->AddControllerYawInput(LookValue.X);
    Character->AddControllerPitchInput(LookValue.Y);
}

void UParcelHeroComponent::StartJump(const FInputActionValue& Value)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!CanProcessLocalInput() || !Character) return;
    
    Character->Jump();

    // 점프 액션 태그 로컬 적용 및 서버 동기화 요청
    ApplyJumpTag(true);
    if (!Character->HasAuthority())
    {
        ServerSetJumping(true);
    }
}

void UParcelHeroComponent::StopJump(const FInputActionValue& Value)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!CanProcessLocalInput() || !Character) return;

    // 물리적인 점프 입력 중단
    Character->StopJumping();

    // 점프 액션 태그 로컬 해제 및 서버 동기화 요청
    ApplyJumpTag(false);
    if (!Character->HasAuthority())
    {
        ServerSetJumping(false);
    }
}

void UParcelHeroComponent::StartSprint(const FInputActionValue& Value)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    URagdollComponent* RagdollComp = Character->FindComponentByClass<URagdollComponent>();
    if (!CanProcessLocalInput() || (RagdollComp && RagdollComp->IsRagdoll())) return;
    
    if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(Character))
    {
        if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
        {
            if (StateComp->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Exhausted"))))
            {
                HEROCOMP_LOG(Warning, TEXT("탈진 상태(Exhausted). 스프린트 불가."));
                return;
            }
        }
    }

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

    // 카메라/틱/메시 가시성 전환은 EnterRagdollCameraMode/ExitRagdollCameraMode가
    // ApplyStartRagdoll/ApplyStopRagdoll 시점에 알아서 처리한다 (해제 쪽은 RecoveryLockDuration만큼 지연됨).
    // 여기서 중복으로 즉시 처리하면 그 지연이 무력화되므로 손대지 않는다.
    RagdollComp->ToggleRagdoll();
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

    // 만약 이미 상자를 들고 있다면 내려놓기(Drop) 실행
    if (UCharacterCarryComponent* CarryComp = Character->FindComponentByClass<UCharacterCarryComponent>())
    {
        if (CarryComp->IsCarrying())
        {
            HEROCOMP_LOG(Log, TEXT("이미 상자를 운반 중: 내려놓기(Drop) 실행"));
            
            // [방어코드] : 던지기 충전 중이었다면 충전 상태 해제
            if (bIsChargingThrow)
            {
                bIsChargingThrow = false;
                CurrentThrowChargeTime = 0.f;
                OnThrowChargeChanged.Broadcast(false, 0.0f);
                
                if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(Character))
                {
                    if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
                    {
                        StateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.Action.Throwing")));
                    }
                }

                if (!RagdollComp || !RagdollComp->IsRagdoll())
                {
                    PrimaryComponentTick.SetTickFunctionEnable(false);
                }
            }
            
            CarryComp->Drop();
            return;
        }
    }

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
    if (bNewIsSprinting)
    {
        if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(GetOwner()))
        {
            if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
            {
                if (StateComp->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Exhausted"))))
                {
                    HEROCOMP_LOG(Warning, TEXT("[Server] %s 가 탈진 중 스프린트 패킷을 발송."), *ParcelChar->GetName());
                    return;
                }
            }
        }
    }
    
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
        
        // MovementStat을 담당하는 매니저에 속도 계산 위임
        if (UParcelMovementStatComponent* StatComp = ParcelChar->GetParcelMovementStatComponent())
        {
            StatComp->RefreshMoveSpeed();
        }
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
	if (SpringArm) SpringArm->TargetArmLength = 350.f;
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
		Character->GetMesh()->SetOwnerNoSee(false);
	PrimaryComponentTick.SetTickFunctionEnable(true);
	HEROCOMP_LOG(Log, TEXT("카메라 래그돌 모드 진입"));
}

void UParcelHeroComponent::ExitRagdollCameraMode()
{
	if (SpringArm) SpringArm->TargetArmLength = 0.f;
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
		Character->GetMesh()->SetOwnerNoSee(true);
	HEROCOMP_LOG(Log, TEXT("카메라 래그돌 모드 해제"));
}

void UParcelHeroComponent::ReattachCameraAfterRagdoll()
{
	// 래그돌 중에는 SpringArm이 매 틱 머리 위치를 따라 SetWorldLocation으로 옮겨져서
	// 캐릭터 루트에서 떨어져 있는 상태다. 기상 애니메이션이 도는 동안 카메라가 허공에
	// 멈춰 있지 않도록, 1인칭 전환(ExitRagdollCameraMode)을 기다리지 않고 즉시
	// 캐릭터에 다시 붙여서 3인칭으로 자연스럽게 따라가게 한다.
	PrimaryComponentTick.SetTickFunctionEnable(false);
	ResetCameraAttachment();
}

void UParcelHeroComponent::StartThrow(const FInputActionValue& Value)
{
    if (!CanProcessLocalInput()) return;

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    UCharacterCarryComponent* CarryComp = Character->FindComponentByClass<UCharacterCarryComponent>();
    if (CarryComp && CarryComp->IsCarrying())
    {
		if (Character->HasAuthority())
		{
			BeginThrowChargeServerOnly();
		}
		else
		{
			Server_BeginThrowCharge();
		}

        bIsChargingThrow = true;
        CurrentThrowChargeTime = 0.f;
        
        // 게이지 모으는 중 틱 활성화
        PrimaryComponentTick.SetTickFunctionEnable(true);
        HEROCOMP_LOG(Log, TEXT("던지기 충전 시작: 틱 활성화"));
        
        // [UI] 던지기 차징 게이지 브로드캐스트
        OnThrowChargeChanged.Broadcast(true, 0.0f);
        
        if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(Character))
        {
            if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
            {
                StateComp->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.Action.Throwing")));
            }
        }
    }
}

void UParcelHeroComponent::ReleaseThrow(const FInputActionValue& Value)
{
    if (!bIsChargingThrow) return;

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    UCharacterCarryComponent* CarryComp = Character->FindComponentByClass<UCharacterCarryComponent>();
    if (CarryComp && CarryComp->IsCarrying())
    {
		const float LocalChargeRatio = MaxThrowChargeTime > KINDA_SMALL_NUMBER
			? FMath::Clamp(CurrentThrowChargeTime / MaxThrowChargeTime, 0.f, 1.f)
			: 0.f;

		if (Character->HasAuthority())
		{
			ReleaseThrowServerOnly();
		}
		else
		{
			Server_ReleaseThrow();
		}

		HEROCOMP_LOG(Log, TEXT("던지기 요청! 로컬 충전 비율: %f (힘은 서버에서 계산)"), LocalChargeRatio);
    }

    // [UI] 던지기 차징 종료 브로드캐스트
    OnThrowChargeChanged.Broadcast(false, 0.0f);
    
    bIsChargingThrow = false;
    CurrentThrowChargeTime = 0.f;
    
    if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(Character))
    {
        if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
        {
            StateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.Action.Throwing")));
        }
    }

    URagdollComponent* RagdollComp = Character->FindComponentByClass<URagdollComponent>();
    bool bNeedsTick = RagdollComp && RagdollComp->IsRagdoll();
    if (!bNeedsTick)
    {
        PrimaryComponentTick.SetTickFunctionEnable(false);
    }
}

void UParcelHeroComponent::Server_BeginThrowCharge_Implementation()
{
	BeginThrowChargeServerOnly();
}

void UParcelHeroComponent::Server_ReleaseThrow_Implementation()
{
	ReleaseThrowServerOnly();
}

void UParcelHeroComponent::BeginThrowChargeServerOnly()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!Character || !Character->HasAuthority() || !World || bServerThrowChargeActive)
	{
		return;
	}

	UCharacterCarryComponent* CarryComp = Character->FindComponentByClass<UCharacterCarryComponent>();
	ADeliveryBox* CarriedBox = CarryComp ? CarryComp->GetCarriedBox() : nullptr;
	if (!CarryComp
		|| !CarryComp->IsCarrying()
		|| !IsValid(CarriedBox)
		|| CarriedBox->GetOwner() != Character)
	{
		return;
	}

	if (const UHealthComponent* HealthComp = Character->FindComponentByClass<UHealthComponent>())
	{
		if (HealthComp->IsDead())
		{
			return;
		}
	}

	if (const URagdollComponent* RagdollComp = Character->FindComponentByClass<URagdollComponent>())
	{
		if (RagdollComp->IsRagdoll())
		{
			return;
		}
	}

	if (!FMath::IsFinite(MinThrowForce)
		|| !FMath::IsFinite(MaxThrowForce)
		|| !FMath::IsFinite(MaxThrowChargeTime)
		|| MinThrowForce < 0.f
		|| MaxThrowForce < MinThrowForce
		|| MaxThrowChargeTime <= KINDA_SMALL_NUMBER)
	{
		HEROCOMP_LOG(Warning, TEXT("[Server] 투척 설정값이 유효하지 않아 충전을 거절했습니다."));
		return;
	}

	bServerThrowChargeActive = true;
	ServerThrowChargeStartTimeSeconds = World->GetTimeSeconds();
	ServerThrowChargeBox = CarriedBox;

	if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(Character))
	{
		if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
		{
			StateComp->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.Action.Throwing")));
		}
	}
}

void UParcelHeroComponent::ReleaseThrowServerOnly()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!Character || !Character->HasAuthority() || !World || !bServerThrowChargeActive)
	{
		return;
	}

	ADeliveryBox* ChargedBox = ServerThrowChargeBox.Get();
	const double ChargeStartTime = ServerThrowChargeStartTimeSeconds;
	CancelServerThrowCharge();

	UCharacterCarryComponent* CarryComp = Character->FindComponentByClass<UCharacterCarryComponent>();
	if (!CarryComp
		|| !CarryComp->IsCarrying()
		|| !IsValid(ChargedBox)
		|| CarryComp->GetCarriedBox() != ChargedBox
		|| ChargedBox->GetOwner() != Character)
	{
		return;
	}

	if (const UHealthComponent* HealthComp = Character->FindComponentByClass<UHealthComponent>())
	{
		if (HealthComp->IsDead())
		{
			return;
		}
	}

	if (const URagdollComponent* RagdollComp = Character->FindComponentByClass<URagdollComponent>())
	{
		if (RagdollComp->IsRagdoll())
		{
			return;
		}
	}

	if (!FMath::IsFinite(MinThrowForce)
		|| !FMath::IsFinite(MaxThrowForce)
		|| !FMath::IsFinite(MaxThrowChargeTime)
		|| MinThrowForce < 0.f
		|| MaxThrowForce < MinThrowForce
		|| MaxThrowChargeTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const double ServerChargeDuration = FMath::Max(0.0, World->GetTimeSeconds() - ChargeStartTime);
	const float ChargeRatio = FMath::Clamp(
		static_cast<float>(ServerChargeDuration / MaxThrowChargeTime),
		0.f,
		1.f);
	const float ForceMagnitude = FMath::Lerp(MinThrowForce, MaxThrowForce, ChargeRatio);

	FVector ThrowDirection = Character->GetControlRotation().Vector();
	ThrowDirection.Z += 0.2f;
	if (!FMath::IsFinite(ThrowDirection.X)
		|| !FMath::IsFinite(ThrowDirection.Y)
		|| !FMath::IsFinite(ThrowDirection.Z)
		|| !ThrowDirection.Normalize())
	{
		return;
	}

	CarryComp->Throw(ThrowDirection * ForceMagnitude);
	HEROCOMP_LOG(Log, TEXT("[Server] 투척 실행: ChargeRatio=%.2f Force=%.2f"), ChargeRatio, ForceMagnitude);
}

void UParcelHeroComponent::CancelServerThrowCharge()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Character->HasAuthority())
	{
		return;
	}

	bServerThrowChargeActive = false;
	ServerThrowChargeStartTimeSeconds = 0.0;
	ServerThrowChargeBox.Reset();

	if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(Character))
	{
		if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
		{
			StateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.Action.Throwing")));
		}
	}
}

void UParcelHeroComponent::ServerSetJumping_Implementation(bool bNewIsJumping)
{
    ApplyJumpTag(bNewIsJumping);
}

void UParcelHeroComponent::ApplyJumpTag(bool bNewIsJumping)
{
    AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(GetOwner());
    if (!ParcelChar) return;

    // 서버 전용 권한 확인 후 중앙 상태 창고 컴포넌트에 실시간 점프 태그 토글 제어
    if (ParcelChar->HasAuthority())
    {
        if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
        {
            FGameplayTag JumpTag = FGameplayTag::RequestGameplayTag(TEXT("Character.Action.Jump"));
            
            if (bNewIsJumping)
            {
                StateComp->AddStateTag(JumpTag);
                HEROCOMP_LOG(Log, TEXT("[Server] 캐릭터에 'Character.Action.Jump' 태그 추가."));
            }
            else
            {
                StateComp->RemoveStateTag(JumpTag);
                HEROCOMP_LOG(Log, TEXT("[Server] 캐릭터의 'Character.Action.Jump' 태그 제거."));
            }
        }
    }
}

void UParcelHeroComponent::UseSlot1(const FInputActionValue& Value) { UseSlot(0); }
void UParcelHeroComponent::UseSlot2(const FInputActionValue& Value) { UseSlot(1); }
void UParcelHeroComponent::UseSlot3(const FInputActionValue& Value) { UseSlot(2); }

void UParcelHeroComponent::UseSlot(int32 SlotIndex)
{
    HEROCOMP_LOG(Log, TEXT("[Client] UseSlot(%d) 입력 감지"), SlotIndex);

    if (!CanProcessLocalInput())
    {
        HEROCOMP_LOG(Warning, TEXT("[Client] UseSlot(%d) 중단: 로컬 입력 불가 (컨트롤러/로컬 여부 확인)"), SlotIndex);
        return;
    }

    AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(GetOwner());
    if (!ParcelChar)
    {
        HEROCOMP_LOG(Warning, TEXT("[Client] UseSlot(%d) 중단: 오너가 ParcelCharacter 아님"), SlotIndex);
        return;
    }

    URagdollComponent* RagdollComp = ParcelChar->FindComponentByClass<URagdollComponent>();
    if (RagdollComp && RagdollComp->IsRagdoll())
    {
        HEROCOMP_LOG(Warning, TEXT("[Client] UseSlot(%d) 중단: 래그돌 상태"), SlotIndex);
        return;
    }

    HEROCOMP_LOG(Log, TEXT("[Client→Server] Server_UseSlot(%d) 전송"), SlotIndex);
    ParcelChar->Server_UseSlot(SlotIndex);
}

void UParcelHeroComponent::ToggleInGameMenu()
{
    UE_LOG(LogTemp, Warning, TEXT("[ESC Test] ToggleInGameMenu 함수가 정상적으로 호출되었습니다!"));

    if (!CanProcessLocalInput()) return;

    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar) return;

    APlayerController* PC = Cast<APlayerController>(OwnerChar->GetController());
    if (!PC) return;
    
    if (ESCMenuRef && ESCMenuRef->IsValidLowLevel() && ESCMenuRef->IsInViewport())
    {
        if (ESCMenuRef->CloseSubMenuIfOpen())
        {
            return;
        }
        
        ESCMenuRef->K2_OnMenuCloseStarted();
        ESCMenuRef = nullptr;
        return;
    }
    
    if (ESCMenuClass)
    {
        ESCMenuRef = CreateWidget<UParcelInGameESCMenuWidget>(PC, ESCMenuClass);
        if (ESCMenuRef)
        {
            ESCMenuRef->AddToViewport();
            ESCMenuRef->SetupMenu();
        }
    }
}

void UParcelHeroComponent::Input_OpenChat()
{
    if (!CanProcessLocalInput()) return;
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    if (AParcelPlayerController* ParcelPC = Cast<AParcelPlayerController>(Character->GetController()))
    {
        if (ParcelPC->LobbyHUDWidgetInstance)
        {
            ParcelPC->LobbyHUDWidgetInstance->SetChatInputInputMode(true);
        }
    }
}

void UParcelHeroComponent::Input_ToggleLobbyMenu()
{
    if (!CanProcessLocalInput()) return;
    
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;
    
    if (AParcelPlayerController* ParcelPC = Cast<AParcelPlayerController>(Character->GetController()))
    {
        if (ParcelPC->LobbyHUDWidgetInstance && ParcelPC->LobbyHUDWidgetInstance->IsValidLowLevel())
        {
            ParcelPC->LobbyHUDWidgetInstance->ToggleLobbyMenuExternal();
            HEROCOMP_LOG(Log, TEXT("[Lobby Menu] 단발성 조작 트리거 ➔ 로비 HUD 토글 신호 직결 완료."));
        }
    }
}
