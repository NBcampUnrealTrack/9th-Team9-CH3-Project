// 이 클래스의 선언을 포함합니다.
#include "Character/ParcelCharacter.h"

// 래그돌 상태 전환과 복제를 담당하는 컴포넌트
#include "Character/RagdollComponent.h"

// 카메라 컴포넌트를 생성하고 설정하기 위해 필요함
#include "Camera/CameraComponent.h"

// Enhanced Input 액션 바인딩을 위해 필요함
#include "EnhancedInputComponent.h"

// Local Player Subsystem에 Input Mapping Context를 등록하기 위해 필요함
#include "EnhancedInputSubsystems.h"

// 캐릭터 이동 속도, 회전, 점프 값을 설정하기 위해 필요함
#include "GameFramework/CharacterMovementComponent.h"

// 현재 캐릭터를 조종하는 PlayerController를 얻기 위해 필요함
#include "GameFramework/PlayerController.h"

// 캐릭터 추적 카메라 암을 생성하고 설정하기 위해 필요함
#include "GameFramework/SpringArmComponent.h"

// 헤더에서 선언한 LogCharacter 로그 카테고리를 실제로 정의합니다.
DEFINE_LOG_CATEGORY(LogCharacter);

namespace
{
	constexpr float WalkSpeed = 450.f;
	constexpr float SprintSpeed = 850.f;
	constexpr float RagdollCameraHeightOffset = 20.f;
	constexpr float RagdollCameraBackOffset = 90.f;
	constexpr float RagdollStopGroundTraceDistance = 120.f;
	constexpr float CameraCollisionProbeSize = 18.f;
}

AParcelCharacter::AParcelCharacter()
{
	// Tick에서 래그돌 중 로컬 카메라 위치를 계속 보정해야 하므로 Tick을 켭니다.
	PrimaryActorTick.bCanEverTick = true;

	// 캐릭터 액터 자체를 네트워크에 복제
	bReplicates = true;

	// 위치, 회전 같은 Actor Movement를 서버에서 클라이언트로 복제
	// ACharacter의 기본 CharacterMovement 복제와 함께 동작합니다.
	SetReplicateMovement(true);

	// 움직임이 잦은 플레이어 캐릭터라서 기본보다 높은 빈도로 네트워크 갱신을 요청
	SetNetUpdateFrequency(100.f);

	// 네트워크 상태가 안정적일 때도 너무 낮은 빈도로 떨어지지 않게 최소 갱신 빈도를 지정합니다.
	SetMinNetUpdateFrequency(33.f);

	// 캐릭터 몸체는 컨트롤러 pitch를 직접 따라가지 않게 합니다.
	// 카메라만 위아래를 볼 수 있게 하기 위한 설정
	bUseControllerRotationPitch = false;

	// 캐릭터 몸체는 컨트롤러 yaw를 직접 따라가지 않게 합니다.
	// 이동 방향 기준으로 자연스럽게 회전하도록 CharacterMovement에 맡깁니다.
	bUseControllerRotationYaw = false;

	// 캐릭터 몸체는 컨트롤러 roll을 직접 따라가지 않게 합니다.
	bUseControllerRotationRoll = false;

	// 이동 입력 방향을 바라보도록 캐릭터를 자동 회전
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// 이동 방향으로 돌아갈 때의 회전 속도.
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

	// 걷기 최대 속도
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// 점프 시작 시 위로 가해지는 속도
	GetCharacterMovement()->JumpZVelocity = 500.f;

	// 공중에서 이동 입력이 얼마나 반영되는지 정합니다.
	GetCharacterMovement()->AirControl = 0.35f;

	// 카메라를 캐릭터에서 일정 거리 떨어뜨리기 위한 SpringArm 컴포넌트를 생성
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));

	// SpringArm을 캐릭터 루트 컴포넌트에 붙입니다.
	SpringArm->SetupAttachment(RootComponent);

	// 캐릭터와 카메라 사이 거리
	SpringArm->TargetArmLength = 350.f;

	// 벽과 지형에 카메라가 파고들지 않도록 SpringArm 충돌 검사를 명시적으로 켭니다.
	SpringArm->bDoCollisionTest = true;
	SpringArm->ProbeChannel = ECC_Camera;
	SpringArm->ProbeSize = CameraCollisionProbeSize;

	// 컨트롤러 회전값을 SpringArm에 적용해서 카메라 방향을 바꿉니다.
	SpringArm->bUsePawnControlRotation = true;

	// 카메라 이동을 즉시 따라가지 않고 약간 부드럽게 따라가게 합니다.
	SpringArm->bEnableCameraLag = true;

	// 카메라 지연 보간 속도
	SpringArm->CameraLagSpeed = 10.f;

	// 실제 화면 렌더링에 쓰는 카메라 컴포넌트를 생성
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));

	// 카메라를 SpringArm 끝 소켓에 붙입니다.
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	// 카메라 자체는 컨트롤러 회전을 다시 적용하지 않습니다.
	// SpringArm이 이미 컨트롤러 회전을 처리합니다.
	FollowCamera->bUsePawnControlRotation = false;

	// 래그돌 상태 전환과 복제를 담당하는 컴포넌트를 생성
	RagdollComp = CreateDefaultSubobject<URagdollComponent>(TEXT("RagdollComp"));
}

void AParcelCharacter::BeginPlay()
{
	// 부모 클래스의 BeginPlay 로직을 먼저 실행
	Super::BeginPlay();

	// 로컬 플레이어가 소유한 캐릭터라면 입력 매핑을 등록
	AddInputMappingContext();
}

void AParcelCharacter::PossessedBy(AController* NewController)
{
	// 서버의 possession 처리 흐름을 유지합니다.
	Super::PossessedBy(NewController);

	// Listen Server의 로컬 플레이어처럼 서버에서도 로컬 입력이 있는 경우를 보강합니다.
	AddInputMappingContext();
}

void AParcelCharacter::OnRep_Controller()
{
	// 클라이언트에서 복제된 Controller 변경 처리를 부모 클래스에 맡깁니다.
	Super::OnRep_Controller();

	// 클라이언트가 Controller 복제를 받은 뒤 입력 매핑을 등록할 수 있게 합니다.
	AddInputMappingContext();
}

void AParcelCharacter::Tick(float DeltaTime)
{
	// 부모 클래스의 Tick 로직을 먼저 실행합니다.
	Super::Tick(DeltaTime);

	// 래그돌 중에는 로컬 플레이어의 카메라 기준점을 머리 소켓 위치로 옮깁니다.
	// 원격 캐릭터의 카메라 컴포넌트를 움직일 필요는 없으므로 로컬 소유 캐릭터만 처리합니다.
	if (IsLocallyControlled() && RagdollComp && RagdollComp->IsRagdoll() && GetMesh() && SpringArm)
	{
		// 머리 소켓의 월드 위치를 가져옵니다.
		const FVector HeadLocation = GetMesh()->GetSocketLocation(TEXT("head"));
		const FRotator ViewRotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
		const FVector CameraBackDirection = -FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::X);
		const FVector TargetLocation =
			HeadLocation +
			FVector::UpVector * RagdollCameraHeightOffset +
			CameraBackDirection * RagdollCameraBackOffset;

		// SpringArm을 몸 밖의 기준점으로 이동시켜 래그돌 중 카메라가 캐릭터 내부로 들어가지 않게 합니다.
		SpringArm->SetWorldLocation(TargetLocation);
	}
}

void AParcelCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 부모 클래스의 입력 설정을 먼저 실행합니다.
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 입력 컴포넌트가 준비되는 시점에도 매핑 등록을 한 번 더 보장합니다.
	AddInputMappingContext();

	// 소유하지 않은 프록시 캐릭터는 입력 바인딩을 만들 필요가 없습니다.
	if (!CanProcessLocalInput())
	{
		return;
	}

	// 일반 UInputComponent를 Enhanced Input 전용 컴포넌트로 캐스팅합니다.
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	// Enhanced Input이 적용되어 있지 않으면 액션 바인딩을 할 수 없습니다.
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogCharacter, Warning, TEXT("EnhancedInputComponent is missing."));
		return;
	}

	// 이동 액션이 지정되어 있으면 입력이 발생하는 동안 Move 함수를 호출합니다.
	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AParcelCharacter::Move);
	}

	// 시점 액션이 지정되어 있으면 입력이 발생하는 동안 Look 함수를 호출합니다.
	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AParcelCharacter::Look);
	}

	// 점프 액션이 지정되어 있으면 시작/종료 이벤트에 각각 점프 시작과 중단을 바인딩합니다.
	if (JumpAction)
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AParcelCharacter::StartJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AParcelCharacter::StopJump);
	}

	// 달리기 액션이 지정되어 있으면 누르는 동안만 달리기 속도를 적용합니다.
	if (SprintAction)
	{
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AParcelCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AParcelCharacter::StopSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AParcelCharacter::StopSprint);
	}

	// 래그돌 액션이 지정되어 있으면 입력 시작 시 래그돌 토글 함수를 호출합니다.
	if (RagdollAction)
	{
		EnhancedInputComponent->BindAction(
			RagdollAction,
			ETriggerEvent::Started,
			this,
			&AParcelCharacter::TestRagdoll
		);
	}
}

void AParcelCharacter::Move(const FInputActionValue& Value)
{
	// 이동 입력은 로컬 소유 캐릭터에서만 처리합니다.
	// ACharacter의 이동 컴포넌트가 서버 보정과 클라이언트 예측을 처리합니다.
	if (!CanProcessLocalInput())
	{
		return;
	}

	// IA_Move의 Axis2D 값을 가져옵니다.
	// X는 좌우, Y는 앞뒤 입력으로 사용합니다.
	const FVector2D MoveValue = Value.Get<FVector2D>();

	// 컨트롤러 회전값을 가져와 카메라가 바라보는 방향을 기준으로 이동 방향을 계산합니다.
	const FRotator ControlRotation = Controller->GetControlRotation();

	// 이동은 수평 방향만 사용하므로 pitch와 roll은 제거하고 yaw만 남깁니다.
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	// yaw 기준의 앞 방향 벡터입니다.
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	// yaw 기준의 오른쪽 방향 벡터입니다.
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// 앞뒤 입력을 캐릭터 이동 입력으로 추가합니다.
	AddMovementInput(ForwardDirection, MoveValue.Y);

	// 좌우 입력을 캐릭터 이동 입력으로 추가
	AddMovementInput(RightDirection, MoveValue.X);
}

void AParcelCharacter::Look(const FInputActionValue& Value)
{
	// 시점 입력은 로컬 소유 캐릭터에서만 처리
	if (!CanProcessLocalInput())
	{
		return;
	}

	// IA_Look의 Axis2D 값을 가져옵니다.
	// X는 좌우 회전, Y는 상하 회전으로 사용.
	const FVector2D LookValue = Value.Get<FVector2D>();

	// 컨트롤러 yaw를 회전시켜 좌우 시점을 변경
	AddControllerYawInput(LookValue.X);

	// 컨트롤러 pitch를 회전시켜 상하 시점을 변경
	AddControllerPitchInput(LookValue.Y);
}

void AParcelCharacter::StartJump(const FInputActionValue& Value)
{
	// 점프 입력은 로컬 소유 캐릭터에서만 처리
	if (!CanProcessLocalInput())
	{
		return;
	}

	// ACharacter 기본 점프 로직을 실행
	Jump();
}

void AParcelCharacter::StopJump(const FInputActionValue& Value)
{
	// 점프 중단 입력은 로컬 소유 캐릭터에서만 처리
	if (!CanProcessLocalInput())
	{
		return;
	}

	// ACharacter 기본 점프 중단 로직을 실행
	StopJumping();
}

void AParcelCharacter::StartSprint(const FInputActionValue& Value)
{
	if (!CanProcessLocalInput() || (RagdollComp && RagdollComp->IsRagdoll()))
	{
		return;
	}

	ApplySprintSpeed(true);

	if (!HasAuthority())
	{
		ServerSetSprinting(true);
	}
}

void AParcelCharacter::StopSprint(const FInputActionValue& Value)
{
	if (!CanProcessLocalInput())
	{
		return;
	}

	ApplySprintSpeed(false);

	if (!HasAuthority())
	{
		ServerSetSprinting(false);
	}
}

void AParcelCharacter::TestRagdoll(const FInputActionValue& Value)
{
	// 래그돌 토글은 로컬 소유 캐릭터 입력에서만 시작합니다.
	// 실제 상태 변경은 URagdollComponent의 서버 RPC를 통해 처리됩니다.
	if (!CanProcessLocalInput() || !RagdollComp)
	{
		return;
	}

	// 토글 전 상태를 저장합니다.
	// 래그돌을 끄는 경우에는 카메라 암을 다시 루트에 붙여야 합니다.
	const bool bWasRagdoll = RagdollComp->IsRagdoll();
	if (bWasRagdoll && !IsRagdollCloseToGround())
	{
		return;
	}

	// 래그돌 컴포넌트에 상태 전환을 요청
	RagdollComp->ToggleRagdoll();

	// 이전 상태가 래그돌이었다면 이번 입력은 래그돌 해제 요청
	if (bWasRagdoll && SpringArm)
	{
		// 래그돌 중 머리 위치로 이동했던 SpringArm을 캐릭터 루트에 다시 붙입니다.
		SpringArm->AttachToComponent(
			RootComponent,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale
		);

		// 루트 기준 위치를 원점으로 되돌립니다.
		SpringArm->SetRelativeLocation(FVector::ZeroVector);
	}
}

void AParcelCharacter::ServerSetSprinting_Implementation(bool bNewIsSprinting)
{
	ApplySprintSpeed(bNewIsSprinting);
}

void AParcelCharacter::ApplySprintSpeed(bool bNewIsSprinting)
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = bNewIsSprinting ? SprintSpeed : WalkSpeed;
	}
}

bool AParcelCharacter::IsRagdollCloseToGround() const
{
	const USkeletalMeshComponent* MeshComponent = GetMesh();
	const UWorld* World = GetWorld();
	if (!MeshComponent || !World)
	{
		return true;
	}

	const FVector TraceStart = MeshComponent->GetSocketLocation(TEXT("pelvis"));
	const FVector TraceEnd = TraceStart - FVector::UpVector * RagdollStopGroundTraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RagdollGroundTrace), false, this);
	FHitResult Hit;
	return World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
}

void AParcelCharacter::AddInputMappingContext()
{
	// 입력 매핑은 로컬 플레이어에게만 등록해야 합니다.
	// 서버의 원격 프록시나 다른 클라이언트의 캐릭터에는 등록하지 않습니다.
	if (!IsLocallyControlled())
	{
		return;
	}

	// 현재 캐릭터를 조종하는 컨트롤러를 플레이어 컨트롤러로 가져옵니다.
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	// Enhanced Input Local Player Subsystem은 LocalPlayer에서 얻을 수 있습니다.
	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	// LocalPlayer에 연결된 Enhanced Input Subsystem을 가져옵니다.
	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	if (!Subsystem)
	{
		return;
	}

	// 에디터나 블루프린트에서 Mapping Context가 지정되지 않은 경우를 경고합니다.
	if (!InputMappingContext)
	{
		UE_LOG(LogCharacter, Warning, TEXT("InputMappingContext is not assigned."));
		return;
	}

	// possession/replication/setup 흐름에서 여러 번 호출될 수 있으므로 먼저 제거합니다.
	Subsystem->RemoveMappingContext(InputMappingContext);

	// 현재 캐릭터의 입력 매핑을 우선순위 0으로 등록합니다.
	Subsystem->AddMappingContext(InputMappingContext, 0);
}

bool AParcelCharacter::CanProcessLocalInput() const
{
	// 컨트롤러가 있고, 이 캐릭터가 현재 머신의 로컬 플레이어에게 소유된 경우에만 입력을 처리합니다.
	return Controller && IsLocallyControlled();
}
