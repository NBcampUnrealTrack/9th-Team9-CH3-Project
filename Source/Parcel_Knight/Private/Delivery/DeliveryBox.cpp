#include "Delivery/DeliveryBox.h"
#include "ParcelLog.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Delivery/PhysicsJudgeManager.h"
#include "Delivery/DeliverySubsystem.h"
#include "Character/CharacterCarryComponent.h"
#include "Core/ParcelPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInterface.h"
#include "Core/HealthComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/ParcelCharacter.h"
#include "Components/DFKnockbackComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Sound/SoundAttenuation.h"
#include "Character/RagdollComponent.h"
#include "Camera/CameraShakeBase.h"

DEFINE_LOG_CATEGORY(LogDeliveryBox);

ADeliveryBox::ADeliveryBox()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false; // 서버에서는 Tick 빌보드 연산 스킵
	bReplicates = true;
	SetReplicateMovement(true); // 리슨 서버 물리 동기화 활성화

	// 박스 콜리전 생성 >> 물리 바디 콜리전 세팅 >> Mesh 부착 후 자체 물리 Off
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;
	
	CollisionComponent->SetSimulatePhysics(true);
	CollisionComponent->SetCollisionProfileName(TEXT("PhysicsBody"));
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	
	// 상자가 가볍게 붕 뜨거나 무한히 굴러다니는 현상을 제어하기 위해 선형/회전 감쇄 적용
	CollisionComponent->SetLinearDamping(0.8f);
	CollisionComponent->SetAngularDamping(1.0f);
	
	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	BoxMesh->SetupAttachment(RootComponent);
	BoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxMesh->SetSimulatePhysics(false);
	
	BoxID = -1;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// 체력 표시용 3D Widget Component 생성 및 기본 옵션 할당
	HPWidgetVisualizer = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPWidgetVisualizer"));
	HPWidgetVisualizer->SetupAttachment(RootComponent);
	HPWidgetVisualizer->SetRelativeLocation(FVector(0.f, 0.f, 50.f)); // 상자 윗부분 Z축 50cm 높이
	HPWidgetVisualizer->SetWidgetSpace(EWidgetSpace::Screen); // 화면 공간 위젯으로 렌더링 (가시성 최고, 글꼴 깨짐 해결)
	HPWidgetVisualizer->SetDrawSize(FVector2D(150.f, 60.f)); // 그릴 위젯 해상도 크기 설정
	HPWidgetVisualizer->SetPivot(FVector2D(0.5f, 0.5f)); // 중앙 정렬 피벗

	// 상자 파쇄 소멸 소리용 기본 감쇄 설정 (ATT_Conveyor) 경로 자동 연결
	static ConstructorHelpers::FObjectFinder<USoundAttenuation> DefaultConveyorAttenuation(TEXT("/Script/Engine.SoundAttenuation'/Game/Delivery/sounds/ATT_Conveyor.ATT_Conveyor'"));
	if (DefaultConveyorAttenuation.Succeeded())
	{
		DestroySoundAttenuation = DefaultConveyorAttenuation.Object;
	}

	// 상자 플레이어 피격 카메라 쉐이크 에셋 (CS_BoxImpact) 경로 자동 연결
	static ConstructorHelpers::FClassFinder<UCameraShakeBase> DefaultImpactCameraShake(TEXT("/Game/UI/Common/CS_BoxImpact"));
	if (DefaultImpactCameraShake.Succeeded())
	{
		BoxImpactCameraShakeClass = DefaultImpactCameraShake.Class;
	}
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

		// 에디터 직접 배치 상자도 에디터 및 뷰포트 물리 시뮬레이션 시 무게 적용되도록 처리
		if (CollisionComponent)
		{
			CollisionComponent->SetMassOverrideInKg(NAME_None, BoxData.Weight * 3.0f, true);
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

	if (HealthComponent)
	{
		HealthComponent->OnHPChanged.AddDynamic(this, &ADeliveryBox::UpdateHPText);
		// 초기 체력 텍스트 갱신
		UpdateHPText(HealthComponent->GetHP(), HealthComponent->GetMaxHP());
	}
}

void ADeliveryBox::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UDeliverySubsystem* DeliverySubsystem = World->GetSubsystem<UDeliverySubsystem>())
			{
				DeliverySubsystem->UnregisterBox(this);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
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
		// 무게 적용 (밸런싱 무게 3.0배 가중치 세팅으로 묵직하게 조절)
		CollisionComponent->SetMassOverrideInKg(NAME_None, BoxData.Weight * 3.0f, true);
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

	// 서버에서 동기화된 BoxData(구역 정보 등)를 받으면 3D 위젯 UI도 최신 구역 정보로 갱신
	if (HealthComponent)
	{
		UpdateHPText(HealthComponent->GetHP(), HealthComponent->GetMaxHP());
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

	// 상자끼리 충돌하거나 포개져 깔아뭉갤 때 서로 대미지를 주거나 연쇄 파손되는 현상 예외 처리
	if (OtherActor && OtherActor->IsA(ADeliveryBox::StaticClass())) return;

	// 언리얼의 NormalImpulse를 그대로 사용하면 상자가 너무 쉽게 부서짐. 따라서 순간 속도 변화량을 역계산하여 사용.
	float ImpactImpulse = NormalImpulse.Size();
	float BoxMass = (BoxData.Weight > 0.0f) ? BoxData.Weight : 1.0f;
	float VelocityChange = (BoxMass > 0.0f) ? (ImpactImpulse / BoxMass) : ImpactImpulse;
	if (VelocityChange < 50.0f) return;
	
	DELIVERYBOX_LOG(Warning, TEXT("[Server] %d번 상자 물리 충돌 발생. 충돌 대상: %s, 계산된 속도 변화량 수치: %f (파손 임계값: %f)"), 
		BoxID, OtherActor ? *OtherActor->GetName() : TEXT("None"), VelocityChange, BoxData.DamageThreshold);

	// 캐릭터가 이 상자에 부딪혔을 때 데미지 처리
	if (OtherActor && OtherActor != this)
	{
		if (AParcelCharacter* HitCharacter = Cast<AParcelCharacter>(OtherActor))
		{
			// 자신이 던진 상자에 즉시 맞는 예외 상황 방지
			bool bIsSelfHit = false;
			if (LastCarrierPlayerState.IsValid())
			{
				if (APawn* LastCarrierPawn = LastCarrierPlayerState->GetPawn())
				{
					if (LastCarrierPawn == HitCharacter)
					{
						bIsSelfHit = true;
					}
				}
			}

			// 자신이 던진 상자가 아니고, 충분히 빠른 속도로 충돌했을 때만 데미지 적용
			if (!bIsSelfHit && VelocityChange >= 150.f)
			{
				// 1) 체력 차감
				if (UHealthComponent* TargetHealth = HitCharacter->FindComponentByClass<UHealthComponent>())
				{
					// 속도 변화량에 따른 데미지 계산 및 클램핑
					float DamageAmount = (VelocityChange - 100.f) * 0.03f;
					DamageAmount = FMath::Clamp(DamageAmount, 5.f, 50.f);

					TargetHealth->TakeDamage(DamageAmount);

					DELIVERYBOX_LOG(Warning, TEXT("[Server] %d번 상자가 플레이어 %s에 충돌하여 %f 데미지를 주었습니다."), 
						BoxID, *HitCharacter->GetName(), DamageAmount);
				}

				// 2) 넉백 효과 적용
				if (UDFKnockbackComponent* TargetKnockback = HitCharacter->FindComponentByClass<UDFKnockbackComponent>())
				{
					float KnockbackStrength = VelocityChange * 1.5f; // 속도 변화량 비례 넉백 세기
					KnockbackStrength = FMath::Clamp(KnockbackStrength, 500.f, 1500.f);
					float UpwardStrength = 300.f; // 붕 뜨게 하는 힘

					TargetKnockback->ApplyKnockbackFromLocation(GetActorLocation(), KnockbackStrength, UpwardStrength);
				}

				// 3) 상자를 들고 있다면 강제로 내려놓게(떨어뜨리게) 처리
				if (UCharacterCarryComponent* TargetCarry = HitCharacter->FindComponentByClass<UCharacterCarryComponent>())
				{
					if (TargetCarry->IsCarrying())
					{
						TargetCarry->ForceDropByTrap(VelocityChange * 0.5f);
					}
				}

				// 4) 즉시 래그돌(Ragdoll) 상태 전환 ➔ 3인칭 카메라 전환 및 이동/입력 조작 차단 연동
				if (URagdollComponent* TargetRagdoll = HitCharacter->GetRagdollComponent())
				{
					TargetRagdoll->StartRagdoll();
				}

				// 5) 피격당한 플레이어의 로컬 화면에 카메라 쉐이크 연동 (CS_BoxImpact)
				if (APlayerController* TargetPC = Cast<APlayerController>(HitCharacter->GetController()))
				{
					if (BoxImpactCameraShakeClass)
					{
						TargetPC->ClientStartCameraShake(BoxImpactCameraShakeClass);
					}
				}
			}
		}
	}

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

	// 모든 클라이언트들에게 파손 소멸 이펙트 재생 요청
	Multicast_PlayDestroyEffect();

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
	if (UWorld* World = GetWorld())
	{
		float CurrentTime = World->GetTimeSeconds();

		// 0) 연속 피격 쿨타임 (0.3초 이내에 연속으로 발생하는 다중 충돌 충격 차단)
		if ((CurrentTime - LastDamageTime) < 0.3f)
		{
			return true;
		}

		float ElapsedTime = CurrentTime - SpawnTime;

		// 1) 이미 플레이어가 한 번 집어 들었다면, 스폰 후 0.5초 경과 시 무적 해제 (던져진 후 충돌 대미지 허용)
		if (bHasBeenPickedUp)
		{
			return ElapsedTime < 0.5f;
		}

		// 2) 주운 적이 없더라도, 스폰된 지 10초가 지나면 무적 자동 해제 (컨베이어 작동 중 가속 충돌 등 파손 가능하게 처리)
		return ElapsedTime < 10.0f;
	}
	return false;
}

void ADeliveryBox::Multicast_PlayDestroyEffect_Implementation()
{
	if (DestroyEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			DestroyEffect,
			GetActorLocation(),
			GetActorRotation(),
			FVector(3.0f) // 이펙트 크기 3배 스케일 업 (기존 1.0f)
		);
	}

	if (DestroySound)
	{
		// 파손 소멸 시 지정한 감쇄 에셋(ATT_Conveyor)을 사용하여 3D 공간음향 재생
		UGameplayStatics::PlaySoundAtLocation(
			this, 
			DestroySound, 
			GetActorLocation(), 
			FRotator::ZeroRotator, 
			1.0f, 
			1.0f, 
			0.0f, 
			DestroySoundAttenuation
		);
	}
}

void ADeliveryBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HPWidgetVisualizer && GetWorld())
	{
		bool bIsHeld = (HoldingCarrier != nullptr) || HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Held")));
		bool bIsDead = (HealthComponent && HealthComponent->GetHP() <= 0.0f);
		bool bIsOccluded = false;

		// 들려있거나 파손된 상태가 아니라면 시선 차폐(LineTrace) 검사 진행
		if (!bIsHeld && !bIsDead)
		{
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				FVector CameraLocation;
				FRotator CameraRotation;
				PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

				FVector TargetWidgetLocation = GetActorLocation() + FVector(0.f, 0.f, 50.f);

				// 거리 제한 (20미터/2000유닛 이상 멀어지면 자동 숨김)
				float Distance = FVector::Dist(CameraLocation, TargetWidgetLocation);
				if (Distance > 2000.f)
				{
					bIsOccluded = true;
				}
				else
				{
					FCollisionQueryParams QueryParams;
					QueryParams.AddIgnoredActor(this);
					if (APawn* LocalPawn = PC->GetPawn())
					{
						QueryParams.AddIgnoredActor(LocalPawn);
					}

					FHitResult HitResult;
					bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, TargetWidgetLocation, ECC_Visibility, QueryParams);
					if (bHit && HitResult.GetActor() && HitResult.GetActor() != this)
					{
						bIsOccluded = true; // 벽이나 구조물에 시선이 가려짐
					}
				}
			}
		}

		// 최종 가시성 결정 (들림, 사망, 벽 가림 상태가 모두 아닐 때만 켬)
		bool bShouldShow = (!bIsHeld) && (!bIsDead) && (!bIsOccluded);
		HPWidgetVisualizer->SetVisibility(bShouldShow);

		// 상자가 뒹굴거나 회전해도 위젯의 위치는 언제나 상자 중심 기준 세계 좌표(World) Z축 방향으로 고정
		FVector BoxLocation = GetActorLocation();
		FVector TargetWidgetLocation = BoxLocation + FVector(0.f, 0.f, 50.f);
		HPWidgetVisualizer->SetWorldLocation(TargetWidgetLocation);
	}
}

void ADeliveryBox::UpdateHPText(float CurrentHP, float MaxHP)
{
	// 체력이 0 이하가 되거나 들려있는 상태면 위젯을 숨김
	if (HPWidgetVisualizer)
	{
		bool bIsHeld = (HoldingCarrier != nullptr) || HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Held")));
		if (CurrentHP <= 0.0f || bIsHeld)
		{
			HPWidgetVisualizer->SetVisibility(false);
			return;
		}
	}

	// 목적지 구역 태그 파싱 (예: "Zone.Type.A" ➔ "A")
	FString ZoneName = TEXT("?");
	if (BoxData.TargetZoneTag.IsValid())
	{
		FString TagString = BoxData.TargetZoneTag.ToString();
		if (TagString.EndsWith(TEXT("A"))) ZoneName = TEXT("A");
		else if (TagString.EndsWith(TEXT("B"))) ZoneName = TEXT("B");
		else if (TagString.EndsWith(TEXT("C"))) ZoneName = TEXT("C");
		else if (TagString.Contains(TEXT("Emergency"))) ZoneName = TEXT("Emergency");
		else
		{
			TArray<FString> OutArray;
			TagString.ParseIntoArray(OutArray, TEXT("."), true);
			if (OutArray.Num() > 0)
			{
				ZoneName = OutArray.Last();
			}
		}
	}

	// 블루프린트 위젯(UMG) 업데이트용 구현 가능 이벤트 호출
	BP_OnHPWidgetUpdated(CurrentHP, MaxHP, ZoneName);
}

void ADeliveryBox::IgnoreThrowerForDuration(AActor* Thrower, float Duration)
{
	if (!Thrower || !CollisionComponent) return;

	CollisionComponent->IgnoreActorWhenMoving(Thrower, true);

	FTimerHandle TempTimerHandle;
	GetWorldTimerManager().SetTimer(
		TempTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, Thrower]()
		{
			if (Thrower && CollisionComponent)
			{
				CollisionComponent->IgnoreActorWhenMoving(Thrower, false);
			}
		}),
		Duration,
		false
	);
}
