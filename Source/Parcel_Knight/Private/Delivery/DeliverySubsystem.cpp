#include "Delivery/DeliverySubsystem.h"
#include "Delivery/DeliveryBox.h"
#include "Delivery/StageData.h"
#include "Engine/DataTable.h"

UDeliverySubsystem::UDeliverySubsystem()
{
    NextBoxID = 1000;
    CurrentStageData = nullptr;
}

void UDeliverySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ActiveBoxes.Empty();
}

void UDeliverySubsystem::Deinitialize()
{
    ActiveBoxes.Empty();
    CurrentStageData = nullptr;
    Super::Deinitialize();
}

void UDeliverySubsystem::InitializeStage(UStageData* InStageData)
{
    if (!InStageData) return;
    
    CurrentStageData = InStageData;
    BoxDataTable = InStageData->BoxDataTable;
    
    DELIVERY_LOG(LogParcelDelivery, Log, TEXT("[Subsystem] 스테이지가 시작되었습니다. 목표 점수: %d"), CurrentStageData->TargetScore);
}

int32 UDeliverySubsystem::GenerateBoxID()
{
    return ++NextBoxID;
}

AActor* UDeliverySubsystem::SpawnBox(FGameplayTag BoxTypeTag, FVector SpawnLocation, FRotator SpawnRotation)
{
    if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client) return nullptr;
    
    if (!BoxDataTable)
    {
        DELIVERY_LOG(LogParcelDelivery, Error, TEXT("[Subsystem] BoxDataTable이 비어있어 상자를 생성할 수 없습니다"));
        return nullptr;
    }

    // 1. 데이터 테이블을 런타임에 순회하며 인자로 들어온 고유 태그와 exact 매칭되는 행(Row) 파싱
    FBoxData* FoundData = nullptr;
    TArray<FBoxData*> AllRows;
    BoxDataTable->GetAllRows<FBoxData>(TEXT(""), AllRows);

    for (FBoxData* RowData : AllRows)
    {
        if (RowData && RowData->BoxTypeTag.MatchesTagExact(BoxTypeTag))
        {
            FoundData = RowData;
            break;
        }
    }

    if (!FoundData)
    {
        DELIVERY_LOG(LogParcelDelivery, Error, TEXT("[Subsystem] 태그 [%s] 에 매칭되는 박스 스펙이 DT_BoxData에 없습니다"), *BoxTypeTag.ToString());
        return nullptr;
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ADeliveryBox* NewBox = GetWorld()->SpawnActor<ADeliveryBox>(ADeliveryBox::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
    if (NewBox)
    {
        int32 AssignedID = GenerateBoxID();
        NewBox->InitializeBox(AssignedID, *FoundData);
        
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