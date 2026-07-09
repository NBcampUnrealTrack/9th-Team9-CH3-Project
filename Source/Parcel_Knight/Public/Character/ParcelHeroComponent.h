#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "ParcelHeroComponent.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PARCEL_KNIGHT_API UParcelHeroComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	UParcelHeroComponent();
	virtual void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// 캐릭터의 입력을 바인딩 (SetupPlayerInputComponent 시점에 호출)
	void InitializePlayerInput(UInputComponent* PlayerInputComponent);
	void AddInputMappingContext();
	
	void ResetCameraAttachment();
	
	void EnterRagdollCameraMode();
	void ExitRagdollCameraMode();
	
protected:
	// 카메라 컴포넌트 (인게임에 시점 변경이 필요하다면 이것도 분리 가능)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
	
	// EnhanceInput (인게임에서 조작 변경이 필요하다면 이것도 분리 가능)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RagdollAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ThrowAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carry|Throw")
	float MinThrowForce = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carry|Throw")
	float MaxThrowForce = 2200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carry|Throw")
	float MaxThrowChargeTime = 1.5f;

	bool bIsChargingThrow = false;
	float CurrentThrowChargeTime = 0.f;
	
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump(const FInputActionValue& Value);
	void StopJump(const FInputActionValue& Value);
	void StartSprint(const FInputActionValue& Value);
	void StopSprint(const FInputActionValue& Value);
	void TestRagdoll(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void StartThrow(const FInputActionValue& Value);
	void ReleaseThrow(const FInputActionValue& Value);
	
	// Ragdoll 카메라 세팅 에디터 노출
	UPROPERTY(EditAnywhere, Category = "Camera|Ragdoll")
	float RagdollCameraHeightOffset = 20.f;

	UPROPERTY(EditAnywhere, Category = "Camera|Ragdoll")
	float RagdollCameraBackOffset = 90.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Collision")
	float CameraCollisionProbeSize = 18.f;
	
private:
	// 달리기 기능을 위한 Server RPC
	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewIsSprinting);

	void ApplySprintSpeed(bool bNewIsSprinting);
	bool CanProcessLocalInput() const;
};
