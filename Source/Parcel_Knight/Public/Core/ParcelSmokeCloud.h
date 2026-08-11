#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParcelSmokeCloud.generated.h"

class UNiagaraComponent;
class USphereComponent;

/**
 * 연막탄 사용 시 스폰되는, 일정 시간 뒤 자동 소멸하는 시야 차단용 연기 액터.
 * FItemData::DeployActorClass에 이 클래스(또는 이 클래스의 블루프린트 자식)를 지정해서 사용한다.
 */
UCLASS()
class PARCEL_KNIGHT_API AParcelSmokeCloud : public AActor
{
	GENERATED_BODY()

public:
	AParcelSmokeCloud();

protected:
	virtual void BeginPlay() override;

	// 연막탄이 사라지기까지 걸리는 시간(초) — 에디터/블루프린트 자식에서 조정 가능
	UPROPERTY(EditDefaultsOnly, Category = "Smoke")
	float Lifetime = 8.0f;

	UPROPERTY(VisibleAnywhere, Category = "Smoke")
	TObjectPtr<USphereComponent> SmokeVolume;

	// 실제 연기 비주얼 — 블루프린트 자식에서 Niagara System 애셋 할당
	UPROPERTY(VisibleAnywhere, Category = "Smoke")
	TObjectPtr<UNiagaraComponent> SmokeFX;

private:
	FTimerHandle LifetimeTimerHandle;

	UFUNCTION()
	void OnLifetimeExpired();
};
