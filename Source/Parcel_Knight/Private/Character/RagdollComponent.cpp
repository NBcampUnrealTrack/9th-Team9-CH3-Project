// 이 컴포넌트의 선언을 포함
#include "Character/RagdollComponent.h"

// 캐릭터 캡슐 충돌을 켜고 끄기 위해 필요
#include "Components/CapsuleComponent.h"

// Skeletal Mesh의 물리 시뮬레이션과 애니메이션 모드를 제어하기 위해 필요
#include "Components/SkeletalMeshComponent.h"

// 소유 액터를 ACharacter로 다루기 위해 필요
#include "GameFramework/Character.h"

// 래그돌 중 캐릭터 이동을 끄고, 복구 시 다시 걷기 모드로 바꾸기 위해 필요
#include "GameFramework/CharacterMovementComponent.h"

// DOREPLIFETIME 매크로를 사용해 프로퍼티 복제를 등록하기 위해 필요
#include "Net/UnrealNetwork.h"

// 헤더에서 선언한 LogRagdoll 로그 카테고리를 실제로 정의
DEFINE_LOG_CATEGORY(LogRagdoll);

URagdollComponent::URagdollComponent()
{
	// 래그돌 상태는 입력이나 복제 이벤트가 들어올 때만 바뀌므로 컴포넌트 Tick은 사용하지 않습니다.
	PrimaryComponentTick.bCanEverTick = false;

	// bIsRagdoll 상태를 서버에서 클라이언트로 복제하기 위해 컴포넌트 복제를 기본 활성화합니다.
	SetIsReplicatedByDefault(true);
}

void URagdollComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// 부모 클래스의 복제 프로퍼티 등록을 먼저 처리합니다.
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 서버에서 결정한 래그돌 상태를 모든 클라이언트로 복제합니다.
	DOREPLIFETIME(URagdollComponent, bIsRagdoll);
}

void URagdollComponent::BeginPlay()
{
	// 부모 클래스의 BeginPlay 로직을 먼저 실행합니다.
	Super::BeginPlay();

	// 이 컴포넌트는 캐릭터에 붙어 있어야 정상 동작하므로 소유자를 캐릭터로 캐싱합니다.
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogRagdoll, Warning, TEXT("Owner is not a Character."));
	}
}

void URagdollComponent::StartRagdoll()
{
	// 클라이언트에서 호출된 경우 직접 상태를 바꾸지 않고 서버에 래그돌 시작을 요청
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		ServerSetRagdoll(true);
		return;
	}

	// 서버 권한이 있으면 실제 래그돌 시작 처리를 바로 적용
	ApplyStartRagdoll();
}

void URagdollComponent::StopRagdoll()
{
	// 클라이언트에서 호출된 경우 직접 상태를 바꾸지 않고 서버에 래그돌 종료를 요청.
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		ServerSetRagdoll(false);
		return;
	}

	// 서버 권한이 있으면 실제 래그돌 종료 처리를 바로 적용
	ApplyStopRagdoll();
}

void URagdollComponent::ToggleRagdoll()
{
	// 클라이언트는 현재 상태의 반대 값을 서버에 요청합니다.
	// 최종 상태 변경은 서버가 처리하고 bIsRagdoll 복제로 클라이언트에 전달됩니다.
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		ServerSetRagdoll(!bIsRagdoll);
		return;
	}

	// 서버에서는 현재 상태에 따라 시작 또는 종료 처리를 결정합니다.
	if (bIsRagdoll)
	{
		ApplyStopRagdoll();
	}
	else
	{
		ApplyStartRagdoll();
	}
}

bool URagdollComponent::IsRagdoll() const
{
	// 현재 캐싱된 래그돌 상태를 반환
	return bIsRagdoll;
}

void URagdollComponent::ServerSetRagdoll_Implementation(bool bNewIsRagdoll)
{
	// 클라이언트 요청을 받은 서버가 최종적으로 래그돌 상태를 적용
	if (bNewIsRagdoll)
	{
		ApplyStartRagdoll();
	}
	else
	{
		ApplyStopRagdoll();
	}
}

void URagdollComponent::OnRep_IsRagdoll()
{
	// 서버에서 복제된 상태를 받은 클라이언트가 같은 래그돌 상태를 로컬에 적용
	if (bIsRagdoll)
	{
		ApplyStartRagdoll();
	}
	else
	{
		ApplyStopRagdoll();
	}
}

void URagdollComponent::ApplyStartRagdoll()
{
	// 소유 캐릭터가 없으면 래그돌을 적용할 대상이 없습니다.
	if (!OwnerCharacter)
	{
		UE_LOG(LogRagdoll, Warning, TEXT("OwnerCharacter is missing."));
		return;
	}

	// 물리 시뮬레이션을 적용할 Skeletal Mesh를 가져옵니다.
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh)
	{
		UE_LOG(LogRagdoll, Warning, TEXT("Character mesh is missing."));
		return;
	}

	// Physics Asset이 없으면 Skeletal Mesh가 래그돌 물리 시뮬레이션을 할 수 없습니다.
	if (!Mesh->GetPhysicsAsset())
	{
		UE_LOG(LogRagdoll, Warning, TEXT("Mesh has no Physics Asset. Ragdoll cannot simulate."));
		return;
	}

	// 래그돌 중에는 CharacterMovement가 캐릭터를 움직이지 않도록 비활성화합니다.
	if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
	{
		Movement->DisableMovement();
	}

	// 캡슐 충돌이 켜져 있으면 물리 중인 메시와 충돌이 겹칠 수 있으므로 끕니다.
	if (UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 애니메이션 블루프린트 제어를 멈추고 메시를 래그돌 충돌 프로파일로 전환합니다.
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// pelvis 아래 본들에 물리 시뮬레이션을 켭니다.
	Mesh->SetAllBodiesBelowSimulatePhysics(TEXT("pelvis"), true, true);

	// pelvis 아래 본들이 물리 결과를 완전히 따르도록 블렌드 가중치를 1로 설정합니다.
	Mesh->SetAllBodiesBelowPhysicsBlendWeight(TEXT("pelvis"), 1.0f, false, true);

	// 잠들어 있는 물리 바디가 있으면 깨워서 바로 시뮬레이션되게 합니다.
	Mesh->WakeAllRigidBodies();

	// 서버에서 바뀐 이 값은 클라이언트로 복제되어 OnRep_IsRagdoll을 호출합니다.
	bIsRagdoll = true;

	UE_LOG(LogRagdoll, Warning, TEXT("Ragdoll started."));
}

void URagdollComponent::ApplyStopRagdoll()
{
	// 소유 캐릭터가 없으면 복구할 대상이 없습니다.
	if (!OwnerCharacter)
	{
		return;
	}

	// 래그돌 물리를 끌 Skeletal Mesh를 가져옵니다.
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh)
	{
		return;
	}

	// 캐릭터 위치 복구와 충돌 복구에 사용할 캡슐 컴포넌트를 가져옵니다.
	UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
	if (!Capsule)
	{
		return;
	}

	// 래그돌 종료 시 캐릭터 루트를 pelvis 위치 근처로 옮겨 순간 이동처럼 보이는 어긋남을 줄입니다.
	FVector PelvisLocation = Mesh->GetSocketLocation(TEXT("pelvis"));

	// 캡슐 중심을 맞추기 위해 캡슐 반 높이를 사용합니다.
	float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	// 전체 메시 물리 시뮬레이션을 끕니다.
	Mesh->SetSimulatePhysics(false);
	Mesh->SetAllBodiesSimulatePhysics(false);
	Mesh->SetAllBodiesBelowSimulatePhysics(TEXT("pelvis"), false, true);

	// 물리 블렌드 상태를 해제합니다.
	Mesh->bBlendPhysics = false;

	// 다시 애니메이션 블루프린트가 메시를 제어하게 합니다.
	Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);

	// pelvis 위치를 기준으로 캡슐 중심 위치를 계산합니다.
	FVector NewActorLocation = PelvisLocation;
	NewActorLocation.Z += CapsuleHalfHeight;

	// 캐릭터 액터를 계산한 위치로 텔레포트해 물리 보정 충돌을 줄입니다.
	OwnerCharacter->SetActorLocation(NewActorLocation, false, nullptr, ETeleportType::TeleportPhysics);

	// 메시를 캡슐 기준 기본 상대 위치로 되돌립니다.
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleHalfHeight), false, nullptr, ETeleportType::TeleportPhysics);

	// 메시를 캐릭터 기본 방향에 맞게 되돌립니다.
	Mesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);

	// 꺼두었던 캡슐 충돌을 다시 켭니다.
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 캐릭터 이동을 다시 걷기 모드로 복구합니다.
	if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}

	// 서버에서 바뀐 이 값은 클라이언트로 복제되어 OnRep_IsRagdoll을 호출합니다.
	bIsRagdoll = false;

	UE_LOG(LogRagdoll, Warning, TEXT("Ragdoll stopped."));
}
