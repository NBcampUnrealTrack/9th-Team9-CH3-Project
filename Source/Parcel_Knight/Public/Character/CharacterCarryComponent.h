// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterCarryComponent.generated.h"

class ADeliveryBox;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PARCEL_KNIGHT_API UCharacterCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCharacterCarryComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// 다이어그램 스펙 맞춤 함수들
	UFUNCTION(BlueprintCallable, Category = "Carry")
	void Pickup(ADeliveryBox* InBox);

	UFUNCTION(BlueprintCallable, Category = "Carry")
	void Drop();

	UFUNCTION(BlueprintCallable, Category = "Carry")
	void Throw(FVector Force);

	UFUNCTION(BlueprintCallable, Category = "Carry")
	void ForceDropByTrap(float TrapDamage);

	// Getter
	UFUNCTION(BlueprintPure, Category = "Carry")
	FORCEINLINE bool IsCarrying() const { return bIsCarrying; }

	UFUNCTION(BlueprintPure, Category = "Carry")
	FORCEINLINE ADeliveryBox* GetCarriedBox() const { return CarriedBox; }

protected:
	// 던지기 처리를 위한 서버 RPC (클라이언트가 던졌을 때 서버에서 물리 적용)
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Throw(FVector Force);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Carry", meta = (AllowPrivateAccess = "true"))
	ADeliveryBox* CarriedBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Carry", meta = (AllowPrivateAccess = "true"))
	bool bIsCarrying;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Carry", meta = (AllowPrivateAccess = "true"))
	float MoveSpeedMultiplier;
};
