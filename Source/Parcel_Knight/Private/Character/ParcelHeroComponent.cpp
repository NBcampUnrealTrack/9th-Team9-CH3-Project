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
#include "Components/DFStatusEffectComponent.h"

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
        
        if (GEngine)
        {
            int32 Percentage = FMath::RoundToInt(ChargeRatio * 100.f);
            FString ProgressBar = TEXT("[");
            int32 BarCount = Percentage / 10;
            for (int32 i = 0; i < 10; ++i)
            {
                ProgressBar += (i < BarCount) ? TEXT("■") : TEXT("□");
            }
            ProgressBar += TEXT("]");

            FString ChargeMsg = FString::Printf(TEXT("던지기 충전 중... %s %d%%"), *ProgressBar, Percentage);
            GEngine->AddOnScreenDebugMessage(8888, 0.1f, FColor::Yellow, ChargeMsg);
        }
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
    
    if (ThrowAction)
    {
       EnhancedInputComponent->BindAction(ThrowAction, ETriggerEvent::Started, this, &UParcelHeroComponent::StartThrow);
       EnhancedInputComponent->BindAction(ThrowAction, ETriggerEvent::Completed, this, &UParcelHeroComponent::ReleaseThrow);
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

    const FVector2D LookValue = Value.Get<FVector2D>();
    Character->AddControllerYawInput(LookValue.X);
    Character->AddControllerPitchInput(LookValue.Y);
}

void UParcelHeroComponent::StartJump(const FInputActionValue& Value)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!CanProcessLocalInput() || !Character) return;

    // 던지기(Throwing) 액션 중에는 물리적인 점프 발동을 차단
    if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(Character))
    {
        if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
        {
            // 점프 시 렉 방어
            FGameplayTag ThrowingAction = FGameplayTag::RequestGameplayTag(TEXT("Character.Action.Throwing"), false);
            FGameplayTag ThrowingState = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Throwing"), false);
            
            // State와 Action 두 태그 명칭 모두 유연하게 검증하도록 방어 코드 작성
            if (StateComp->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.Action.Throwing"))) ||
                StateComp->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Throwing")))) 
            {
                return;
            }
        }
    }

    // 물리적인 점프 기능 수행
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
	PrimaryComponentTick.SetTickFunctionEnable(true);
	HEROCOMP_LOG(Log, TEXT("카메라 래그돌 모드 진입"));
}

void UParcelHeroComponent::ExitRagdollCameraMode()
{
	PrimaryComponentTick.SetTickFunctionEnable(false);
	ResetCameraAttachment();
	HEROCOMP_LOG(Log, TEXT("카메라 래그돌 모드 해제"));
}

void UParcelHeroComponent::StartThrow(const FInputActionValue& Value)
{
    if (!CanProcessLocalInput()) return;

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    UCharacterCarryComponent* CarryComp = Character->FindComponentByClass<UCharacterCarryComponent>();
    if (CarryComp && CarryComp->IsCarrying())
    {
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
        float ChargeRatio = FMath::Clamp(CurrentThrowChargeTime / MaxThrowChargeTime, 0.f, 1.f);
        float ForceMag = FMath::Lerp(MinThrowForce, MaxThrowForce, ChargeRatio);
        
        // 카메라의 조준 방향 계산 (약간 위로 향해 포물선을 그리도록 보정)
        FVector ThrowDir = FollowCamera->GetForwardVector();
        ThrowDir.Z += 0.2f;
        ThrowDir.Normalize();
        
        FVector ThrowForce = ThrowDir * ForceMag;
        CarryComp->Throw(ThrowForce);
        HEROCOMP_LOG(Log, TEXT("던지기 실행! 충전 비율: %f, 최종 힘: %f"), ChargeRatio, ForceMag);
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