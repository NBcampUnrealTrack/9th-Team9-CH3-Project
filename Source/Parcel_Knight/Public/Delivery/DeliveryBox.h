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
class UMaterialInterface;
class UHealthComponent;

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
	virtual bool CanCarry_Implementation(AActor* Carrier) override;
	virtual void OnPickedUp_Implementation(AActor* Carrier) override;
	virtual void OnDropped_Implementation() override;
	
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
	
	// 스폰 초기 무적 상태 여부 확인 (0.5초 무적)
	UFUNCTION(BlueprintCallable, Category = "Delivery")
	bool IsInvulnerable() const;
	
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHealthComponent> HealthComponent;
	
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

protected:
	// 각 구역 태그별로 머티리얼을 매핑할 수 있는 맵 (A, B, C 구역별 색상 지정용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Delivery Box Material")
	TMap<FGameplayTag, TObjectPtr<UMaterialInterface>> ZoneMaterials;

private:
	// 상자 목적지 구역 태그에 맞춰 머티리얼을 동적으로 적용합니다.
	void ApplyZoneMaterial();

	UFUNCTION()
	void HandleOnDeath();

	// 상자 스폰 시점의 게임 시간 (초 단위)
	float SpawnTime = 0.0f;

	// 상자가 한 번이라도 플레이어에게 주워졌는지 여부 (주워지기 전까지 무적 처리용)
	bool bHasBeenPickedUp = false;
};