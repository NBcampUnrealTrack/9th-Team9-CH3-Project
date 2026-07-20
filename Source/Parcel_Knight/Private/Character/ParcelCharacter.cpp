#include "Character/ParcelCharacter.h"
#include "ParcelLog.h"
#include "Character/RagdollComponent.h"
#include "Character/ParcelHeroComponent.h"
#include "Character/ParcelInteractionComponent.h"
#include "Character/ParcelMovementStatComponent.h"
#include "Character/CharacterCarryComponent.h"
#include "Character/ParcelPlayerStateComponent.h"
#include "Character/ParcelStaminaComponent.h"
#include "Components/DFKnockbackComponent.h"
#include "Components/DFStatusEffectComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Components/WidgetComponent.h"
#include "Core/HealthComponent.h"
#include "Core/ParcelPlayerState.h"
#include "Core/InventoryComponent.h"
#include "Core/CustomizationComponent.h"
#include "UI/ParcelNameplateWidget.h"
#include "Data/ItemData.h"


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
	StaminaComp = CreateDefaultSubobject<UParcelStaminaComponent>(TEXT("StaminaComp"));
	NameplateWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameplateWidgetComp"));
	if (NameplateWidgetComp)
	{
		NameplateWidgetComp->SetupAttachment(GetMesh());
		NameplateWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
		NameplateWidgetComp->SetDrawSize(FVector2D(250.f, 80.f));
		NameplateWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 210.f));
	}
	if (PlayerStateComp)
	{
		PlayerStateComp->OnCharacterStateTagsChanged.AddUniqueDynamic(this, &AParcelCharacter::OnCharacterStateTagsChanged);
	}

	bIsRagdoll = false;
	bIsGettingUp = false;
}

void AParcelCharacter::BeginPlay()
{
    Super::BeginPlay();
	
	if (PlayerStateComp)
	{
		PlayerStateComp->OnCharacterStateTagsChanged.AddUniqueDynamic(this, &AParcelCharacter::OnCharacterStateTagsChanged);
	}
	
	// 사망 로직 보완
	if (UHealthComponent* HealthComp = FindComponentByClass<UHealthComponent>())
	{
		HealthComp->OnDeathDelegate.RemoveDynamic(this, &AParcelCharacter::HandleCharacterDeath);
		HealthComp->OnDeathDelegate.AddUniqueDynamic(this, &AParcelCharacter::HandleCharacterDeath);
	}
	
	if (GetWorld())
	{
		FTimerHandle StandaloneNameplateTimer;
		GetWorldTimerManager().SetTimer(
			StandaloneNameplateTimer, 
			this, 
			&AParcelCharacter::UpdateOverheadNameplate, 
			0.2f, // 0.2초 뒤 안정적으로 데이터가 로드되었을 때 실행
			false
		);
	}
}

void AParcelCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (HasAuthority() && PlayerStateComp)
	{
		FGameplayTag InAirTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.InAir"));

		if (GetCharacterMovement()->MovementMode == MOVE_Falling)
		{
			PlayerStateComp->AddStateTag(InAirTag);
			PLAYER_LOG(All, TEXT("[Server] %s 캐릭터가 공중 상태(MOVE_Falling)로 진입했습니다. (InAir 태그 추가)"), *GetName());
		}
	}
}

void AParcelCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();
	if (HasAuthority() && PlayerStateComp)
	{
		PlayerStateComp->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.InAir")));
	}
}

void AParcelCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
    
	if (HasAuthority() && PlayerStateComp)
	{
		// 1. 공중 체공 상태 태그 해제
		FGameplayTag InAirTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.InAir"));
		if (PlayerStateComp->HasStateTag(InAirTag))
		{
			PlayerStateComp->RemoveStateTag(InAirTag);
			PLAYER_LOG(All, TEXT("[Server] %s 캐릭터가 지면에 착지했습니다. (InAir 태그 제거)"), *GetName());
		}

		// [안전장치] 착지했으므로 혹시라도 지워지지 않고 남아있을 점프 액션 태그를 확실하게 청소
		PlayerStateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.Action.Jump")));
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
	
	if (HasAuthority() && PlayerStateComp)
	{
		FGameplayTag RagdollTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Ragdoll"));
		FGameplayTag GettingUpTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.GettingUp"));

		if (bIsRagdoll) PlayerStateComp->AddStateTag(RagdollTag);
		else            PlayerStateComp->RemoveStateTag(RagdollTag);

		if (bIsGettingUp) PlayerStateComp->AddStateTag(GettingUpTag);
		else              PlayerStateComp->RemoveStateTag(GettingUpTag);
	}

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

	// [보완] 일어서기 타이머(기상 몽타주)가 끝나면 서버에서 GettingUp 태그 확실히 회수
	if (HasAuthority() && PlayerStateComp)
	{
		PlayerStateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.GettingUp")));
	}
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

	if (UHealthComponent* HealthComp = FindComponentByClass<UHealthComponent>())
    {
        if (AParcelPlayerState* PS = GetPlayerState<AParcelPlayerState>())
        {
            HealthComp->OnDeathDelegate.AddUniqueDynamic(PS, &AParcelPlayerState::HandleDeath);
        }
        HealthComp->OnDeathDelegate.AddUniqueDynamic(this, &AParcelCharacter::OnCharacterDeath);
    }

	if (AParcelPlayerState* PS = GetPlayerState<AParcelPlayerState>())
	{
		if (UInventoryComponent* InvComp = PS->GetInventoryComponent())
			InvComp->ApplyPassiveEffects(this);

		if (UCustomizationComponent* CustComp = PS->GetCustomizationComponent())
			ApplyTitle(CustComp->GetEquippedTitle());
	}

	UpdateOverheadNameplate();
}

void AParcelCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	UpdateOverheadNameplate();
}

void AParcelCharacter::UpdateOverheadNameplate()
{
	APlayerState* PS = GetPlayerState();
	if (!PS)
	{
		if (!NameplateRetryTimerHandle.IsValid() && GetWorld())
		{
			GetWorldTimerManager().SetTimer(
				NameplateRetryTimerHandle, 
				this, 
				&AParcelCharacter::UpdateOverheadNameplate, 
				0.1f, // 0.1초 뒤 다시 들어와서 PlayerState 검사
				false
			);
		}
		return;
	}

	if (NameplateWidgetComp)
	{
		if (UParcelNameplateWidget* NameWidget = Cast<UParcelNameplateWidget>(NameplateWidgetComp->GetUserWidgetObject()))
		{
			FString Nickname = PS->GetPlayerName();
			NameWidget->SetPlayerName(Nickname);
			
			if (PlayerStateComp)
			{
				NameWidget->UpdateStatusEffects(PlayerStateComp->GetCharacterStateTags());
			}

			GetWorldTimerManager().ClearTimer(NameplateRetryTimerHandle);
            
			PLAYER_LOG(All, TEXT("[%s] 머리 위 네임플레이트 및 상태이상 연동 완료."), *GetName(), *Nickname);
		}
		else
		{
			if (!NameplateRetryTimerHandle.IsValid() && GetWorld())
			{
				GetWorldTimerManager().SetTimer(
					NameplateRetryTimerHandle, 
					this, 
					&AParcelCharacter::UpdateOverheadNameplate, 
					0.1f, 
					false
				);
			}
		}
	}
}

void AParcelCharacter::OnCharacterStateTagsChanged(const FGameplayTagContainer& ActiveTags)
{
	if (NameplateWidgetComp)
	{
		if (UParcelNameplateWidget* NameWidget = Cast<UParcelNameplateWidget>(NameplateWidgetComp->GetUserWidgetObject()))
		{
			NameWidget->UpdateStatusEffects(ActiveTags);
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

void AParcelCharacter::OnCharacterDeath()
{
	if (RagdollComp)
		RagdollComp->StartRagdoll();
}

void AParcelCharacter::Server_UseSlot_Implementation(int32 SlotIndex)
{
	PLAYER_LOG(Log, TEXT("[Server] Server_UseSlot(%d) 수신"), SlotIndex);

	AParcelPlayerState* PS = GetPlayerState<AParcelPlayerState>();
	if (!PS)
	{
		PLAYER_LOG(Warning, TEXT("[Server] UseSlot(%d) 중단: PlayerState null"), SlotIndex);
		return;
	}

	UInventoryComponent* InvComp = PS->GetInventoryComponent();
	if (!InvComp)
	{
		PLAYER_LOG(Warning, TEXT("[Server] UseSlot(%d) 중단: InventoryComponent null"), SlotIndex);
		return;
	}

	const TArray<FGameplayTag>& Items = InvComp->GetItems();
	PLAYER_LOG(Log, TEXT("[Server] 인벤토리 크기=%d, 요청 슬롯=%d"), Items.Num(), SlotIndex);

	if (!Items.IsValidIndex(SlotIndex))
	{
		PLAYER_LOG(Warning, TEXT("[Server] UseSlot(%d) 중단: 유효하지 않은 슬롯 인덱스"), SlotIndex);
		return;
	}

	FGameplayTag ItemTag = Items[SlotIndex];
	PLAYER_LOG(Log, TEXT("[Server] 슬롯[%d] = %s"), SlotIndex, *ItemTag.ToString());

	static const FGameplayTag TAG_Consumable = FGameplayTag::RequestGameplayTag(TEXT("Item.Consumables"));
	static const FGameplayTag TAG_Gun        = FGameplayTag::RequestGameplayTag(TEXT("Item.Consumables.Gun"));

	if (ItemTag.MatchesTag(TAG_Consumable))
	{
		if (!InvComp->UseItem(ItemTag))
		{
			PLAYER_LOG(Warning, TEXT("[Server] UseItem(%s) 실패 — 자세한 원인은 LogItem 확인"), *ItemTag.ToString());
			return;
		}
		if (ItemTag == TAG_Gun)
		{
			PLAYER_LOG(Log, TEXT("[Server] Gun 라인트레이스 실행"));
			DoGunLineTrace();
		}
	}
	else
	{
		PLAYER_LOG(Warning, TEXT("[Server] 슬롯[%d] 아이템 '%s'이 Item.Consumables 태그 계층에 속하지 않음"), SlotIndex, *ItemTag.ToString());
	}
	// Item.Cosmetic.* — CustomizationComponent 연동 추후 구현
}

void AParcelCharacter::DoGunLineTrace()
{
	AController* Ctrl = GetController();
	if (!Ctrl) return;

	FVector ViewLoc;
	FRotator ViewRot;
	Ctrl->GetPlayerViewPoint(ViewLoc, ViewRot);
	FVector End = ViewLoc + ViewRot.Vector() * 10000.f;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLoc, End, ECC_Pawn, Params))
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			if (UHealthComponent* HC = HitActor->FindComponentByClass<UHealthComponent>())
				HC->TakeDamage(99999.f);
		}
	}
}

void AParcelCharacter::ApplyTitle(FGameplayTag TitleTag)
{
	UParcelNameplateWidget* NameWidget = nullptr;
	if (NameplateWidgetComp)
		NameWidget = Cast<UParcelNameplateWidget>(NameplateWidgetComp->GetUserWidgetObject());
	if (!NameWidget) return;

	if (!TitleTag.IsValid() || !CosmeticDataTable)
	{
		NameWidget->SetTitle(nullptr);
		return;
	}

	TArray<FItemData*> AllRows;
	CosmeticDataTable->GetAllRows<FItemData>(TEXT("ApplyTitle"), AllRows);
	for (FItemData* Row : AllRows)
	{
		if (Row && Row->ItemTag == TitleTag)
		{
			NameWidget->SetTitle(Row);
			return;
		}
	}

	NameWidget->SetTitle(nullptr);
}

void AParcelCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AParcelCharacter::HandleCharacterDeath()
{
	PLAYER_LOG(All, TEXT("[사망 체인] 캐릭터 사망 로직이 정상 가동됩니다."));

	// Ragdoll 활성화
	if (RagdollComp)
	{
		RagdollComp->StartRagdoll();
	}

	// Dead 상태 태그
	if (HasAuthority() && PlayerStateComp)
	{
		PlayerStateComp->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Dead")));
	}

	// PlayerState로 넘김
	if (HasAuthority())
	{
		if (AParcelPlayerState* PS = GetPlayerState<AParcelPlayerState>())
		{
			PS->HandleDeath();
		}
	}
}