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
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/TextureRenderTarget2D.h"

DEFINE_LOG_CATEGORY(LogCharacter);

AParcelCharacter::AParcelCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicateMovement(true);
    bReplicates = true;

    SetNetUpdateFrequency(33.f);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;

    // 공중에서 이동 입력이 얼마나 반영되는지 정합니다.
    GetCharacterMovement()->AirControl = 0.35f;

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

    // 미니맵용 스프링 암 및 씬 캡처 컴포넌트 생성 및 설정
	MinimapSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("MinimapSpringArm"));
	if (MinimapSpringArm)
	{
		MinimapSpringArm->SetupAttachment(RootComponent);
		MinimapSpringArm->TargetArmLength = 800.f;
		MinimapSpringArm->bUsePawnControlRotation = false;
		MinimapSpringArm->bInheritPitch = false;
		MinimapSpringArm->bInheritRoll = false;
		MinimapSpringArm->bInheritYaw = false;
		MinimapSpringArm->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
		MinimapSpringArm->bDoCollisionTest = false;
	}

	// 미니맵 씬 캡처 컴포넌트 설정
	MinimapCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapCaptureComponent"));
	if (MinimapCaptureComponent)
	{
		MinimapCaptureComponent->SetupAttachment(MinimapSpringArm);
		MinimapCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
		MinimapCaptureComponent->OrthoWidth = 2500.f;
		MinimapCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		MinimapCaptureComponent->bCaptureEveryFrame = true;
		MinimapCaptureComponent->bCaptureOnMovement = true;
		MinimapCaptureComponent->bAutoActivate = false;
	}

    bIsRagdoll = false;
    bIsGettingUp = false;
}

void AParcelCharacter::BeginPlay()
{
    Super::BeginPlay();

    UpdateMinimapCaptureState();

    GetMesh()->SetOwnerNoSee(true);
	
	if (PlayerStateComp)
	{
		PlayerStateComp->OnCharacterStateTagsChanged.AddUniqueDynamic(this, &AParcelCharacter::OnCharacterStateTagsChanged);
	}
	
	BindAuthoritativeDeathHandler();
	
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
       FGameplayTag InAirTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.InAir"));
       if (PlayerStateComp->HasStateTag(InAirTag)) PlayerStateComp->RemoveStateTag(InAirTag);
       PlayerStateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.Action.Jump")));
    }
    if (MovementStatComp) MovementStatComp->RefreshMoveSpeed();
}

void AParcelCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (HeroComp) HeroComp->InitializePlayerInput(PlayerInputComponent);
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
       GetWorldTimerManager().SetTimer(GetUpTimerHandle, this, &AParcelCharacter::FinishGetUp, 1.3f, false);
    }
    else
    {
       GetWorldTimerManager().ClearTimer(GetUpTimerHandle);
    }
}

void AParcelCharacter::FinishGetUp()
{
    bIsGettingUp = false;
    if (HasAuthority() && PlayerStateComp)
    {
       PlayerStateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.GettingUp")));
    }
}

void AParcelCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    UpdateMinimapCaptureState();

    // 서버 Possessed 시점에 HeroComponent에 이벤트를 넘김
    if (HeroComp)
    {
       HeroComp->AddInputMappingContext();
    }

	BindAuthoritativeDeathHandler();

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
          GetWorldTimerManager().SetTimer(NameplateRetryTimerHandle, this, &AParcelCharacter::UpdateOverheadNameplate, 0.1f, false);
       }
       return;
    }

    if (NameplateWidgetComp)
    {
       if (UParcelNameplateWidget* NameWidget = Cast<UParcelNameplateWidget>(NameplateWidgetComp->GetUserWidgetObject()))
       {
          FString Nickname = PS->GetPlayerName();
          NameWidget->SetPlayerName(Nickname);
          if (PlayerStateComp) NameWidget->UpdateStatusEffects(PlayerStateComp->GetCharacterStateTags());
          GetWorldTimerManager().ClearTimer(NameplateRetryTimerHandle);
       }
       else
       {
          if (!NameplateRetryTimerHandle.IsValid() && GetWorld())
          {
             GetWorldTimerManager().SetTimer(NameplateRetryTimerHandle, this, &AParcelCharacter::UpdateOverheadNameplate, 0.1f, false);
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
    Super::OnRep_Controller();
    
    UpdateMinimapCaptureState();

    if (HeroComp) HeroComp->AddInputMappingContext();
}

void AParcelCharacter::OnCharacterDeath()
{
	HandleCharacterDeath();
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
	if (!HasAuthority() || bDeathHandled)
	{
		return;
	}

	bDeathHandled = true;

    if (RagdollComp) RagdollComp->StartRagdoll();
    if (PlayerStateComp)
    {
        PlayerStateComp->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Dead")));
    }

	if (AParcelPlayerState* PS = GetPlayerState<AParcelPlayerState>())
    {
		PS->HandleDeath();
    }
}

void AParcelCharacter::BindAuthoritativeDeathHandler()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UHealthComponent* HealthComp = FindComponentByClass<UHealthComponent>())
	{
		// 이전 병렬 경로를 제거하고 서버의 단일 진입점만 유지한다.
		HealthComp->OnDeathDelegate.RemoveDynamic(this, &AParcelCharacter::OnCharacterDeath);
		if (AParcelPlayerState* PS = GetPlayerState<AParcelPlayerState>())
		{
			HealthComp->OnDeathDelegate.RemoveDynamic(PS, &AParcelPlayerState::HandleDeath);
		}

		HealthComp->OnDeathDelegate.RemoveDynamic(this, &AParcelCharacter::HandleCharacterDeath);
		HealthComp->OnDeathDelegate.AddUniqueDynamic(this, &AParcelCharacter::HandleCharacterDeath);
	}
}

void AParcelCharacter::UpdateMinimapCaptureState()
{
	if (!GetController())
	{
		return;
	}
	
	if (IsLocallyControlled())
	{
		if (MinimapCaptureComponent)
		{
			// 로컬 전용 동적 렌더 타겟 안전 생성
			if (!DynamicMinimapRenderTarget && GetWorld())
			{
				DynamicMinimapRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
				   this, 512, 512, ETextureRenderTargetFormat::RTF_RGBA8
				);
			}

			if (DynamicMinimapRenderTarget)
			{
				MinimapCaptureComponent->TextureTarget = DynamicMinimapRenderTarget;
				MinimapCaptureComponent->Activate(true);
				MinimapCaptureComponent->SetComponentTickEnabled(true);
				MinimapCaptureComponent->CaptureScene();
			}
		}
	}
	else
	{
		if (MinimapCaptureComponent)
		{
			MinimapCaptureComponent->TextureTarget = nullptr;
			MinimapCaptureComponent->Deactivate();
			MinimapCaptureComponent->SetComponentTickEnabled(false);
		}
	}
}

UTextureRenderTarget2D* AParcelCharacter::GetMinimapRenderTarget()
{
	if (!DynamicMinimapRenderTarget && IsLocallyControlled())
	{
		UpdateMinimapCaptureState();
	}
	return DynamicMinimapRenderTarget;
}
