#pragma once

// 엔진에서 자주 쓰는 기본 타입, 매크로, 컨테이너를 포함
#include "CoreMinimal.h"

// ACharacter를 상속받기 위해 필요한 헤더
#include "GameFramework/Character.h"

// UnrealHeaderTool이 생성하는 리플렉션 코드를 포함
#include "ParcelCharacter.generated.h"

// 이 캐릭터 클래스 전용 로그 카테고리
DECLARE_LOG_CATEGORY_EXTERN(LogCharacter, Log, All);

// 헤더 의존성을 줄이기 위한 전방 선언
class URagdollComponent;
class UParcelHeroComponent;

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

	// 래그돌 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<URagdollComponent> RagdollComp;
	
	// HeroComponent
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UParcelHeroComponent> HeroComp;
};
