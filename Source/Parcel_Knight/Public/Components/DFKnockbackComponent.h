#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DFKnockbackComponent.generated.h"

class ACharacter;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UDFKnockbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFKnockbackComponent();

	UFUNCTION(BlueprintCallable, Category = "Knockback")
	void ApplyKnockbackFromLocation(FVector SourceLocation, float Strength, float UpwardStrength);

	UFUNCTION(BlueprintCallable, Category = "Knockback")
	void ApplyKnockback(FVector Direction, float Strength, float UpwardStrength);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION(Server, Reliable)
	void Server_ApplyKnockbackFromLocation(FVector SourceLocation, float Strength, float UpwardStrength);

	UFUNCTION(Server, Reliable)
	void Server_ApplyKnockback(FVector Direction, float Strength, float UpwardStrength);

	ACharacter* GetOwnerCharacter();

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;
};
