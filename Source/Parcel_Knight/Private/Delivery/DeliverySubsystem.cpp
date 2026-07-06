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

void UDeliverySubsystem::DespawnBox(AActor* Box)
{
    if (!Box) return;
    ActiveBoxes.Remove(Box);
    Box->Destroy();
}