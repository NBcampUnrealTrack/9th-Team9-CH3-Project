#pragma once

// 엔진에서 자주 쓰는 기본 타입, 매크로, 컨테이너를 포함
#include "CoreMinimal.h"

// ACharacter를 상속받기 위해 필요한 헤더
#include "GameFramework/Character.h"

// Enhanced Input 액션 값(FInputActionValue)을 함수 인자로 사용하기 위해 포함
#include "InputActionValue.h"

// UnrealHeaderTool이 생성하는 리플렉션 코드를 포함
#include "ParcelCharacter.generated.h"

// 이 캐릭터 클래스 전용 로그 카테고리
DECLARE_LOG_CATEGORY_EXTERN(LogCharacter, Log, All);

// 헤더 의존성을 줄이기 위한 전방 선언
class AController;
class UCameraComponent;
class URagdollComponent;
class UInputAction;
class UInputComponent;
class UInputMappingContext;
class USpringArmComponent;

// 플레이어가 조종하는 기본 캐릭터 클래스
// 이동, 시점 회전, 점프, 래그돌 테스트 입력을 처리하고 멀티플레이 복제를 지원
//
// 담당자: 김로운
UCLASS()
class PARCEL_KNIGHT_API AParcelCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// 기본 컴포넌트, 이동 설정, 네트워크 복제 설정을 초기화
	AParcelCharacter();

	// 매 프레임 호출됩니다. 현재는 로컬 래그돌 카메라 위치 보정에 사용
	virtual void Tick(float DeltaTime) override;

	// 서버에서 이 캐릭터가 컨트롤러에 빙의될 때 호출
	// Listen Server의 로컬 플레이어 입력 매핑 등록을 보강
	virtual void PossessedBy(AController* NewController) override;

	// 클라이언트에서 Controller 값이 복제되어 바뀔 때 호출됩니다.
	// 원격 접속 클라이언트가 possession 이후 입력 매핑을 놓치지 않게 합니다.
	virtual void OnRep_Controller() override;

protected:
	// 게임 시작 시 호출됩니다. 로컬 플레이어라면 Enhanced Input 매핑을 등록합니다.
	virtual void BeginPlay() override;

	// 플레이어 입력 컴포넌트에 Enhanced Input 액션들을 바인딩합니다.
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// 캐릭터 뒤를 따라가는 카메라 암입니다.
	// 컨트롤러 회전을 사용해서 카메라 방향을 정합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArm;

	// 실제 화면을 렌더링하는 추적 카메라입니다.
	// SpringArm 끝 소켓에 붙어 있습니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	// 래그돌 시작/종료와 상태 복제를 담당하는 커스텀 컴포넌트.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	URagdollComponent* RagdollComp;

	// 이 캐릭터가 사용할 Enhanced Input Mapping Context입니다.
	// 블루프린트나 에디터에서 지정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* InputMappingContext;

	// WASD, 왼쪽 스틱 등 이동 입력 액션입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	// 마우스, 오른쪽 스틱 등 시점 회전 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	// 점프 시작/종료 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	// 누르고 있는 동안 달리기 속도로 전환하는 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SprintAction;

	// 래그돌 상태를 토글하는 테스트 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* RagdollAction;

	// MoveAction에서 들어온 2D 입력값으로 캐릭터 이동 입력을 추가
	void Move(const FInputActionValue& Value);

	// LookAction에서 들어온 2D 입력값으로 컨트롤러 yaw/pitch를 회전
	void Look(const FInputActionValue& Value);

	// JumpAction이 시작될 때 캐릭터 점프를 시작
	void StartJump(const FInputActionValue& Value);

	// JumpAction이 끝날 때 점프 입력을 중단
	void StopJump(const FInputActionValue& Value);

	// SprintAction이 시작될 때 이동 속도를 달리기 속도로 바꿉니다.
	void StartSprint(const FInputActionValue& Value);

	// SprintAction이 끝날 때 이동 속도를 걷기 속도로 되돌립니다.
	void StopSprint(const FInputActionValue& Value);

	// RagdollAction이 시작될 때 래그돌 상태를 토글합니다.
	void TestRagdoll(const FInputActionValue& Value);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewIsSprinting);

	void ApplySprintSpeed(bool bNewIsSprinting);

	bool IsRagdollCloseToGround() const;

	// 현재 캐릭터를 소유한 로컬 플레이어에게 Enhanced Input Mapping Context를 등록
	void AddInputMappingContext();

	// 이 인스턴스가 입력을 처리해도 되는 로컬 소유 캐릭터인지 확인합니다.
	bool CanProcessLocalInput() const;
};
