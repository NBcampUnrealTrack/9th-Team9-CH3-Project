#include "Delivery/DeliverySubsystem.h"
#include "ParcelLog.h"
#include "Delivery/DeliveryBox.h"
#include "Delivery/StageData.h"
#include "Engine/DataTable.h"

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
        NewBox->InitializeBox(AssignedID, FoundData);
        
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