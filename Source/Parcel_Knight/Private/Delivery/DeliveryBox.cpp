#include "Delivery/DeliveryBox.h"
#include "ParcelLog.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Delivery/PhysicsJudgeManager.h"
#include "Character/CharacterCarryComponent.h"
#include "Core/ParcelPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInterface.h"
#include "Core/HealthComponent.h"

DEFINE_LOG_CATEGORY(LogDeliveryBox);

ADeliveryBox::ADeliveryBox()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true); // 리슨 서버 물리 동기화 활성화

	// 박스 콜리전 생성 >> 물리 바디 콜리전 세팅 >> Mesh 부착 후 자체 물리 Off
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;
	
	CollisionComponent->SetSimulatePhysics(true);
	CollisionComponent->SetCollisionProfileName(TEXT("PhysicsBody"));
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	
	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	BoxMesh->SetupAttachment(RootComponent);
	BoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxMesh->SetSimulatePhysics(false);
	
	BoxID = -1;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

void ADeliveryBox::BeginPlay()
{
	Super::BeginPlay();

	SpawnTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	
	// 원래 서브시스템에서 스폰되어야 하는데, 에디터에서 직접 드래그해서 배치하는 경우 사용할 값
	if (BoxID == -1)
	{
		BoxData.Weight = 50.f;
		BoxData.DamageThreshold = 500.f;
        
		if (BoxStateTags.IsEmpty())
		{
			BoxStateTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Spawned")));
		}

		// 에디터 배치용 테스트 상자도 생성 시점에 체력 컴포넌트 값을 안전하게 초기화해 줍니다.
		if (HealthComponent)
		{
			HealthComponent->InitializeHP(BoxData.DamageThreshold);
		}
	}
	
	if (HasAuthority() && CollisionComponent)
	{
		CollisionComponent->OnComponentHit.AddDynamic(this, &ADeliveryBox::OnPhysicsHit);
	}

	if (HasAuthority() && HealthComponent)
	{
		HealthComponent->OnDeathDelegate.AddDynamic(this, &ADeliveryBox::HandleOnDeath);
	}
}

void ADeliveryBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADeliveryBox, BoxID);
	DOREPLIFETIME(ADeliveryBox, BoxStateTags);
	DOREPLIFETIME(ADeliveryBox, BoxData);
	DOREPLIFETIME(ADeliveryBox, HoldingCarrier);
}

void ADeliveryBox::InitializeBox(int32 InBoxID, const FBoxData& InBoxData)
{
	if (!HasAuthority()) return;

	SpawnTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	BoxID = InBoxID;
	BoxData = InBoxData;
	
	if (BoxData.BoxMeshAsset && BoxMesh && CollisionComponent)
	{
		BoxMesh->SetStaticMesh(BoxData.BoxMeshAsset);
		// 무게 적용 (밸런싱 수치 조절 (현재 1.0f))
		CollisionComponent->SetMassOverrideInKg(NAME_None, BoxData.Weight * 1.0f, true);
	}
	
	// 목적지 구역에 맞는 색상 머티리얼 적용
	ApplyZoneMaterial();

	// 상자 체력 컴포넌트의 초기/최대 체력을 데이터 테이블 임계값 정보로 매핑하여 초기화
	if (HealthComponent)
	{
		HealthComponent->InitializeHP(BoxData.DamageThreshold);
	}

	// Spawned 태그 부여
	AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Spawned")));
	
	DELIVERYBOX_LOG(Log, TEXT("[Server] 상자 고유ID %d번 초기화 완료. 타입 태그: %s, 설정 무게: %f kg"), 
		BoxID, *BoxData.BoxTypeTag.ToString(), BoxData.Weight);
}

void ADeliveryBox::OnRep_BoxData()
{
	// 클라이언트 측에서도 서버가 채워준 BoxData를 받으면 메시 외형을 동기화함
	if (BoxData.BoxMeshAsset && BoxMesh)
	{
		BoxMesh->SetStaticMesh(BoxData.BoxMeshAsset);
		
		// 목적지 구역에 맞는 색상 머티리얼 적용
		ApplyZoneMaterial();
	}
	
	DELIVERYBOX_LOG(Log, TEXT("[Client] %d번 상자의 외형 데이터 동기화 완료. 상자 타입 태그: %s"), 
	   BoxID, *BoxData.BoxTypeTag.ToString());
}

void ADeliveryBox::AddStateTag(FGameplayTag NewStateTag)
{
	if (!HasAuthority() || !NewStateTag.IsValid()) return;

	if (!BoxStateTags.HasTagExact(NewStateTag))
	{
		BoxStateTags.AddTag(NewStateTag);
		
		DELIVERYBOX_LOG(Log, TEXT("[Server] %d번 상자에 새로운 상태 태그 추가됨: %s"), BoxID, *NewStateTag.ToString());
	
		//플레이어가 상자를 들면 상자의 물리 규칙은 잠시 꺼야 함.
		if (NewStateTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Held"))))
		{
			CollisionComponent->SetSimulatePhysics(false);
			CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ADeliveryBox::RemoveStateTag(FGameplayTag StateTag)
{
	if (!HasAuthority() || !StateTag.IsValid()) return;

	if (BoxStateTags.HasTagExact(StateTag))
	{
		BoxStateTags.RemoveTag(StateTag);
        
		DELIVERYBOX_LOG(Log, TEXT("[Server] %d번 상자에서 상태 태그 제거됨: %s"), BoxID, *StateTag.ToString());
		
		// 상태가 제거될 때의 예외 복구 로직
		if (StateTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Held"))))
		{
			CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			CollisionComponent->SetSimulatePhysics(true);
		}
	}
}

void ADeliveryBox::OnRep_BoxStateTags()
{
	FGameplayTag DamagedTag = FGameplayTag::RequestGameplayTag(TEXT("Box.State.Damaged"));

	DELIVERYBOX_LOG(Log, TEXT("[Client] %d번 상자의 상태 태그 컨테이너 갱신됨. 현재 태그 목록: %s"), 
	   BoxID, *BoxStateTags.ToString());
    
	// 클라이언트 사이드 물리 콜리전 켜고 끄기는 CharacterCarryComponent가 담당하도록 이전
	if (BoxStateTags.HasTag(DamagedTag))
	{
		// Todo : 찌그러진 메쉬로 교체하거나 내용물이 튀어나오는 이펙트/사운드 호출
	}
}

/* ==========================================================================
   ICarryableInterface 인터페이스
   ========================================================================== */

bool ADeliveryBox::CanCarry_Implementation(AActor* Carrier)
{
	if (!BoxStateTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Spawned")))) return false;
	if (Carrier)
	{
		// 방어 코드
		if (UCharacterCarryComponent* CarryComp = Carrier->FindComponentByClass<UCharacterCarryComponent>())
		{
			if (CarryComp->IsCarrying())
			{
				return false;
			}
		}
	}
	return true;
}

void ADeliveryBox::OnPickedUp_Implementation(AActor* Carrier)
{
	if (!HasAuthority() || !Carrier) return;
    
	if (APawn* CarrierPawn = Cast<APawn>(Carrier))
	{
		HoldingCarrier = CarrierPawn;
		SetOwner(CarrierPawn);
       
		if (APlayerController* PC = Cast<APlayerController>(CarrierPawn->GetController()))
		{
			LastCarrierPlayerState = PC->GetPlayerState<AParcelPlayerState>();
		}

		DELIVERYBOX_LOG(Log, TEXT("[Server] %d번 상자 획득 처리 완료. 소유 캐릭터: %s"), 
		  BoxID, *HoldingCarrier->GetName());
	}

	RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Spawned")));
	AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Held")));

	// 플레이어가 최초로 주워 들었으므로 이제 무적 처리를 해제할 수 있습니다.
	bHasBeenPickedUp = true;
}

void ADeliveryBox::OnDropped_Implementation()
{
	if (!HasAuthority()) return;

	DELIVERYBOX_LOG(Log, TEXT("[Server] %d번 상자 낙하 처리 완료."), BoxID);
    
	HoldingCarrier = nullptr;
	SetOwner(nullptr); 

	RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Held")));
	AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Spawned")));
}

void ADeliveryBox::OnPhysicsHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority()) return;
	if (HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Damaged")))) return;
	if (HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Held")))) return;
	if (IsInvulnerable()) return;

	// 언리얼의 NormalImpulse를 그대로 사용하면 상자가 너무 쉽게 부서짐. 따라서 순간 속도 변화량을 역계산하여 사용.
	float ImpactImpulse = NormalImpulse.Size();
	float BoxMass = (BoxData.Weight > 0.0f) ? BoxData.Weight : 1.0f;
	float VelocityChange = (BoxMass > 0.0f) ? (ImpactImpulse / BoxMass) : ImpactImpulse;
	if (VelocityChange < 50.0f) return;
	
	DELIVERYBOX_LOG(Warning, TEXT("[Server] %d번 상자 물리 충돌 발생. 충돌 대상: %s, 계산된 속도 변화량 수치: %f (파손 임계값: %f)"), 
		BoxID, OtherActor ? *OtherActor->GetName() : TEXT("None"), VelocityChange, BoxData.DamageThreshold);

	if (UWorld* World = GetWorld())
	{
		if (UPhysicsJudgeManager* DamageManager = World->GetSubsystem<UPhysicsJudgeManager>())
		{
		   DamageManager->EvaluateImpact(this, VelocityChange);
		}
	}
}

/* ==========================================================================
   IInteractableInterface 인터페이스
   ========================================================================== */

bool ADeliveryBox::CanInteract_Implementation(AActor* Interactor)
{
	return ICarryableInterface::Execute_CanCarry(this, Interactor);
}

void ADeliveryBox::Interact_Implementation(AActor* Interactor)
{
	if (!Interactor) return;
    
	// 상태를 Held 태그로 바꿈
	ICarryableInterface::Execute_OnPickedUp(this, Interactor);
    
	// CharacterCarryComponent 호출하여 손에 붙임
	if (UCharacterCarryComponent* CharacterCarryComp = Interactor->FindComponentByClass<UCharacterCarryComponent>())
	{
		CharacterCarryComp->Pickup(this);
	}
}

void ADeliveryBox::ApplyZoneMaterial()
{
	if (!BoxMesh || !BoxData.TargetZoneTag.IsValid()) return;

	if (TObjectPtr<UMaterialInterface>* FoundMaterial = ZoneMaterials.Find(BoxData.TargetZoneTag))
	{
		if (*FoundMaterial)
		{
			BoxMesh->SetMaterial(0, *FoundMaterial);
			DELIVERYBOX_LOG(Log, TEXT("[Material] 상자 ID %d번의 목적지 구역 %s에 맞는 머티리얼을 적용했습니다."), 
				BoxID, *BoxData.TargetZoneTag.ToString());
		}
	}
}

void ADeliveryBox::HandleOnDeath()
{
	if (!HasAuthority()) return;

	AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Damaged")));

	if (UWorld* World = GetWorld())
	{
		if (UPhysicsJudgeManager* JudgeManager = World->GetSubsystem<UPhysicsJudgeManager>())
		{
			JudgeManager->OnBoxDamaged.Broadcast(this);
		}
	}

	DELIVERYBOX_LOG(Warning, TEXT("[Server] 상자 ID %d번 체력(HP)이 0이 되어 맵에서 소멸 처리되었습니다."), BoxID);

	// 상자를 파손 즉시 맵에서 소멸시킴
	Destroy();
}

bool ADeliveryBox::IsInvulnerable() const
{
	// 플레이어가 상자를 최소 한 번 집어 올리기 전까지는 월드 물리/함정 충격 대미지에 대해 100% 무적 처리
	if (!bHasBeenPickedUp) return true;

	if (UWorld* World = GetWorld())
	{
		return (World->GetTimeSeconds() - SpawnTime) < 0.5f;
	}
	return false;
}