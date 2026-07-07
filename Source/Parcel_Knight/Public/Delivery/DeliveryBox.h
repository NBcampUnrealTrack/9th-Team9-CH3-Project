#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "DeliveryTypes.h"
#include "Delivery/CarryableInterface.h"
#include "Delivery/InteractableInterface.h"
#include "DeliveryBox.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class PARCEL_KNIGHT_API ADeliveryBox : public AActor, public ICarryableInterface, public IInteractableInterface
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

    // [Interface] ICarryableInterface 오버라이드
    virtual bool CanCarry(AActor* Carrier) override;
    virtual void OnPickedUp(AActor* Carrier) override;
    virtual void OnDropped() override;
	
	// [Interface] IInteractableInterface 오버라이드
	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	FORCEINLINE TWeakObjectPtr<class AParcelPlayerState> GetLastCarrierPlayerState() const { return LastCarrierPlayerState; }
	

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
	
	UPROPERTY(ReplicatedUsing = OnRep_BoxStateTags, EditDefaultsOnly, BlueprintReadOnly, Category = "Delivery", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer BoxStateTags;
	
	UPROPERTY(ReplicatedUsing = OnRep_BoxData)
	FBoxData BoxData;
	
	// Pawn 을 복제해서 모든 유저가 들기 상태를 인지
	UPROPERTY(Replicated)
	TObjectPtr<APawn> HoldingCarrier;

	// [Server] 서버에서 스코어링 판정용 PlayerState 캐시
	TWeakObjectPtr<AParcelPlayerState> LastCarrierPlayerState;
};