#pragma once

#include "CoreMinimal.h"
#include "UI/ParcelSessionWidget.h"
#include "ParcelMainMenuWidget.generated.h"

class UButton;
class UParcelOptionsWidget;

/**
 * UParcelMainMenuWidget
 * 담당자 : JYW
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelMainMenuWidget : public UParcelSessionWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// UMG 위젯 버튼 바인딩
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_SinglePlay;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_MultiPlay;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Options;

	// 상점은 아직 기믹 미구현이므로 Optional 처리
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Shop;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_ExitGame;

	// 재활용 및 생성할 위젯 클래스 정보
	UPROPERTY(EditDefaultsOnly, Category = "MainMenu|UI")
	TSubclassOf<UParcelOptionsWidget> OptionsWidgetClass;

	// 블루프린트 UI 연출용 이벤트
	/** 멀티플레이 버튼 클릭 시 RoomList 위젯 슬라이드 다운 애니메이션 재생을 요청 */
	UFUNCTION(BlueprintImplementableEvent, Category = "MainMenu|Events")
	void K2_OnMultiPlayMenuOpened();

private:
	// 버튼 클릭 핸들러 함수
	UFUNCTION()
	void HandleSinglePlayClicked();

	UFUNCTION()
	void HandleMultiPlayClicked();

	UFUNCTION()
	void HandleOptionsClicked();

	UFUNCTION()
	void HandleShopClicked();

	UFUNCTION()
	void HandleExitGameClicked();
};
