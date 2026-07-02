#include "Character/ParcelHeroComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Character/RagdollComponent.h"

// Todo : 하드코딩 요소 제거 필요함.
namespace
{
	constexpr float WalkSpeed = 450.f;
	constexpr float SprintSpeed = 850.f;
	constexpr float RagdollCameraHeightOffset = 20.f;
	constexpr float RagdollCameraBackOffset = 90.f;
	constexpr float RagdollStopGroundTraceDistance = 120.f;
	constexpr float CameraCollisionProbeSize = 18.f;
}

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
	}

	AddInputMappingContext();
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
}

void UParcelHeroComponent::Move(const FInputActionValue& Value)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!CanProcessLocalInput() || !Character || !Character->GetController()) return;

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
    if (CanProcessLocalInput() && Character) Character->Jump();
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
	if (bWasRagdoll && !IsRagdollCloseToGround()) return;

	RagdollComp->ToggleRagdoll();

	// 래그돌이 켜질 때만 컴포넌트 틱을 킴(Tick 최적화)
	if (RagdollComp->IsRagdoll())
	{
		PrimaryComponentTick.SetTickFunctionEnable(true);
	}
	else
	{
		PrimaryComponentTick.SetTickFunctionEnable(false);
		if (SpringArm)
		{
			SpringArm->AttachToComponent(Character->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			SpringArm->SetRelativeLocation(FVector::ZeroVector);
		}
	}
}

void UParcelHeroComponent::ServerSetSprinting_Implementation(bool bNewIsSprinting)
{
    ApplySprintSpeed(bNewIsSprinting);
}

void UParcelHeroComponent::ApplySprintSpeed(bool bNewIsSprinting)
{
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
        {
           Movement->MaxWalkSpeed = bNewIsSprinting ? SprintSpeed : WalkSpeed;
        }
    }
}

bool UParcelHeroComponent::IsRagdollCloseToGround() const
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character || !GetWorld()) return true;

    const USkeletalMeshComponent* MeshComponent = Character->GetMesh();
    if (!MeshComponent) return true;

    const FVector TraceStart = MeshComponent->GetSocketLocation(TEXT("pelvis"));
    const FVector TraceEnd = TraceStart - FVector::UpVector * RagdollStopGroundTraceDistance;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RagdollGroundTrace), false, Character);
    FHitResult Hit;
    return GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
}

void UParcelHeroComponent::AddInputMappingContext()
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character || !Character->IsLocallyControlled()) return;

    APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
    if (!PlayerController) return;

    ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
    if (!LocalPlayer) return;

    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
    if (!Subsystem) return;

    if (!InputMappingContext) return;

    Subsystem->RemoveMappingContext(InputMappingContext);
    Subsystem->AddMappingContext(InputMappingContext, 0);
}

bool UParcelHeroComponent::CanProcessLocalInput() const
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    return Character && Character->GetController() && Character->IsLocallyControlled();
}