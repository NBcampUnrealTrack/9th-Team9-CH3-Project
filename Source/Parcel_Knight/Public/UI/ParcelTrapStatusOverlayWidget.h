#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParcelTrapStatusOverlayWidget.generated.h"

class UBorder;

UCLASS()
class PARCEL_KNIGHT_API UParcelTrapStatusOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Trap|Feedback")
	void ShowTrapStatus(FLinearColor Color, float Duration);

	UFUNCTION(BlueprintCallable, Category = "Trap|Feedback")
	void HideTrapStatus();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> TrapEdgeBorder;

private:
	FTimerHandle HideTimerHandle;
};
