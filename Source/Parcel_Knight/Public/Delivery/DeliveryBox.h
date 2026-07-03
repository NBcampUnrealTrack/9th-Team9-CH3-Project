#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "DeliveryTypes.h"
#include "Delivery/Carryable.h"
#include "Delivery/Interactable.h"
#include "DeliveryBox.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogParcelDelivery, Log, All);

#define DELIVERY_LOG(Category, Verbosity, Format, ...) UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__)

class UBoxComponent;
class UStaticMeshComponent;
class UCarryComponent;

UCLASS()
class PARCEL_KNIGHT_API ADeliveryBox : public AActor, public ICarryable, public IInteractable
{
	GENERATED_BODY()
	
public:	
	ADeliveryBox();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;

public:
    // 서버에서 상태 태그를 제어하는 권한 전용 함수
    void AddStateTag(FGameplayTag NewStateTag);
    void RemoveStateTag(FGameplayTag StateTag);
	
    void InitializeBox(int32 InBoxID, const FBoxData& InBoxData);

    // [Interface] ICarryable 오버라이드
    virtual bool CanCarry(AActor* Carrier) override;
    virtual void OnPickedUp(AActor* Carrier) override;
    virtual void OnDropped() override;
	
	// [Interface] IInteractable 오버라이드
	virtual bool CanInteract(AActor* Interactor) override;
	virtual void Interact(AActor* Interactor) override;
	

    // Getter 함수. 상자 ID, Tag, BoxData, 물리 임계값
	FORCEINLINE bool HasStateTag(FGameplayTag StateTag) const { return BoxStateTags.HasTag(StateTag); }
	
    FORCEINLINE int32 GetBoxID() const { return BoxID; }
    FORCEINLINE FGameplayTagContainer GetBoxStateTags() const { return BoxStateTags; }
    FORCEINLINE FBoxData GetBoxData() const { return BoxData; }
    FORCEINLINE float GetDamageThreshold() const { return BoxData.DamageThreshold; }
	
protected:
	// [Client] 태그 변경 시 클라이언트 연출용 RepNotify
	UFUNCTION()
	void OnRep_BoxStateTags();

	// [Client] 데이터 설정 시 클라이언트 Mesh 동기화용 RepNotify
	UFUNCTION()
	void OnRep_BoxData();
	
	// [Server] 서버에서 호출될 파손 판정용 콜백 함수
	UFUNCTION()
	void OnPhysicsHit(
		UPrimitiveComponent* HitComponent, 
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		const FVector NormalImpulse,
		const FHitResult& Hit);
	
private:
	// 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> CollisionComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BoxMesh;
	
	/* ==========================================================================
		   동기화 규칙 변수 (Replicated)
	========================================================================== */
	UPROPERTY(Replicated)
	int32 BoxID;
	
	UPROPERTY(ReplicatedUsing = OnRep_BoxStateTags)
	FGameplayTagContainer BoxStateTags;
	
	UPROPERTY(ReplicatedUsing = OnRep_BoxData)
	FBoxData BoxData;
	
	UPROPERTY(Replicated)
	TObjectPtr<APlayerController> HolderPlayer;
};