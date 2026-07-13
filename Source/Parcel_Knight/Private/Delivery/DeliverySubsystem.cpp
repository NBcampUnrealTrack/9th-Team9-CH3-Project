#include "Delivery/DeliverySubsystem.h"
#include "ParcelLog.h"
#include "Delivery/DeliveryBox.h"
#include "Delivery/StageData.h"
#include "Engine/DataTable.h"
#include "Delivery/DeliveryZone.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogDeliverySubsystem);

UDeliverySubsystem::UDeliverySubsystem()
{
    NextBoxID = 1000;
    CurrentStageData = nullptr;
}

void UDeliverySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ActiveBoxes.Empty();
    CachedBoxData.Empty();
}

void UDeliverySubsystem::Deinitialize()
{
    ActiveBoxes.Empty();
    CachedBoxData.Empty();
    CurrentStageData = nullptr;
    Super::Deinitialize();
}

void UDeliverySubsystem::InitializeStage(UStageData* InStageData)
{
    if (!InStageData) return;
    
    CurrentStageData = InStageData;
    BoxDataTable = InStageData->BoxDataTable;

    CachedBoxData.Empty();
    if (BoxDataTable)
    {
        TArray<FBoxData*> AllRows;
        BoxDataTable->GetAllRows<FBoxData>(TEXT(""), AllRows);
        for (FBoxData* RowData : AllRows)
        {
            if (RowData)
            {
                CachedBoxData.Add(RowData->BoxTypeTag, *RowData);
            }
        }
    }
    
    DELIVERYSUBSYSTEM_LOG(Log, TEXT("[Subsystem] 스테이지가 시작되었습니다. 목표 점수: %d"), CurrentStageData->TargetScore);
}

int32 UDeliverySubsystem::GenerateBoxID()
{
    return ++NextBoxID;
}

AActor* UDeliverySubsystem::SpawnBox(FGameplayTag BoxTypeTag, FVector SpawnLocation, FRotator SpawnRotation)
{
    if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client) return nullptr;
    
    EnsureCacheLoaded();
    
    const FBoxData* FoundDataPtr = CachedBoxData.Find(BoxTypeTag);
    if (!FoundDataPtr)
    {
        DELIVERYSUBSYSTEM_LOG(Error, TEXT("[Subsystem] 태그 [%s] 에 매칭되는 박스 스펙을 캐시에서 찾을 수 없습니다"), *BoxTypeTag.ToString());
        return nullptr;
    }

    const FBoxData& FoundData = *FoundDataPtr;
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // BoxClass가 지정되어 있으면 사용하고, 없으면 기본 ADeliveryBox 클래스 사용 (삼항 연산자 애매함 해소)
    TSubclassOf<ADeliveryBox> SpawnClass = ADeliveryBox::StaticClass();
    if (FoundData.BoxClass)
    {
        SpawnClass = FoundData.BoxClass;
    }

    ADeliveryBox* NewBox = GetWorld()->SpawnActor<ADeliveryBox>(SpawnClass, SpawnLocation, SpawnRotation, SpawnParams);
    if (NewBox)
    {
        int32 AssignedID = GenerateBoxID();

        // 박스 기본 데이터 복사
        FBoxData RandomizedData = FoundData;

        // 만약 긴급 배송 상자(Zone.Type.Emergency)가 아니라면, 현재 레벨에 배치된 배송 구역(DeliveryZone)들의 태그 중 무작위로 매핑합니다.
        FGameplayTag EmergencyZoneTag = FGameplayTag::RequestGameplayTag(TEXT("Zone.Type.Emergency"));
        if (!FoundData.TargetZoneTag.MatchesTagExact(EmergencyZoneTag))
        {
            TArray<AActor*> FoundZones;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADeliveryZone::StaticClass(), FoundZones);

            TArray<FGameplayTag> ActiveZoneTags;
            for (AActor* ZoneActor : FoundZones)
            {
                if (ADeliveryZone* Zone = Cast<ADeliveryZone>(ZoneActor))
                {
                    if (Zone->ZoneTag.IsValid() && !Zone->ZoneTag.MatchesTagExact(EmergencyZoneTag))
                    {
                        ActiveZoneTags.AddUnique(Zone->ZoneTag);
                    }
                }
            }

            // 배치된 배송 구역이 존재할 때만 그 중에서 무작위로 선택 (예: 2개가 배치되면 2개 중 하나)
            if (ActiveZoneTags.Num() > 0)
            {
                int32 RandomIndex = FMath::RandRange(0, ActiveZoneTags.Num() - 1);
                RandomizedData.TargetZoneTag = ActiveZoneTags[RandomIndex];
            }
            else
            {
                // 월드에 배치된 구역이 없는 경우에 대비한 기본 폴백 (A, B, C 중 랜덤 선택)
                TArray<FString> ZoneTagStrings = { TEXT("Zone.Type.A"), TEXT("Zone.Type.B"), TEXT("Zone.Type.C") };
                int32 RandomZoneIndex = FMath::RandRange(0, ZoneTagStrings.Num() - 1);
                RandomizedData.TargetZoneTag = FGameplayTag::RequestGameplayTag(*ZoneTagStrings[RandomZoneIndex]);
            }
        }

        // 랜덤화된 정보로 상자 초기화
        NewBox->InitializeBox(AssignedID, RandomizedData);
        
        ActiveBoxes.Add(NewBox);
        return NewBox;
    }

    return nullptr;
}

AActor* UDeliverySubsystem::SpawnRandomBox(FVector SpawnLocation, FRotator SpawnRotation)
{
	EnsureCacheLoaded();

	if (CachedBoxData.IsEmpty())
	{
		DELIVERYSUBSYSTEM_LOG(Warning, TEXT("[Subsystem] 캐시된 박스 데이터가 없어 랜덤 스폰이 불가능합니다."));
		return nullptr;
	}

	TArray<FGameplayTag> Keys;
	CachedBoxData.GetKeys(Keys);

	int32 RandomIndex = FMath::RandRange(0, Keys.Num() - 1);
	FGameplayTag SelectedTag = Keys[RandomIndex];

	return SpawnBox(SelectedTag, SpawnLocation, SpawnRotation);
}

void UDeliverySubsystem::DespawnBox(AActor* Box)
{
    if (!Box) return;
    
    if (!Box->HasAuthority()) return;
    
    ActiveBoxes.Remove(Box);
    Box->Destroy();
}

void UDeliverySubsystem::EnsureCacheLoaded()
{
	if (!CachedBoxData.IsEmpty()) return;

	// 테스트용 폴백: 스테이지 데이터가 로드되지 않은 상태에서 테스트할 수 있도록 기본 데이터 테이블을 자동 로드합니다.
	UDataTable* DefaultTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/Data/DataTables/DT_BoxData")));
	if (DefaultTable)
	{
		TArray<FBoxData*> AllRows;
		DefaultTable->GetAllRows<FBoxData>(TEXT(""), AllRows);
		for (FBoxData* RowData : AllRows)
		{
			if (RowData)
			{
				CachedBoxData.Add(RowData->BoxTypeTag, *RowData);
			}
		}
		DELIVERYSUBSYSTEM_LOG(Log, TEXT("[Subsystem] 테스트용 기본 데이터 테이블(DT_BoxData)을 자동으로 로드하여 캐싱했습니다."));
	}
}