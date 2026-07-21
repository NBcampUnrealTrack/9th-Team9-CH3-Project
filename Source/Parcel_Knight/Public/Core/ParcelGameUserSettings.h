#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "ParcelGameUserSettings.generated.h"

class USoundClass;
class USoundMix;

UCLASS(Config = GameUserSettings)
class PARCEL_KNIGHT_API UParcelGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UParcelGameUserSettings();

	virtual void SetToDefaults() override;
	virtual void LoadSettings(bool bForceReload = false) override;
	virtual void ValidateSettings() override;

	UFUNCTION(BlueprintPure, Category = "Parcel|Settings")
	static UParcelGameUserSettings* GetParcelGameUserSettings();

	UFUNCTION(BlueprintPure, Category = "Parcel|Settings|Audio")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintCallable, Category = "Parcel|Settings|Audio")
	void SetMasterVolume(float NewMasterVolume);

	UFUNCTION(BlueprintCallable, Category = "Parcel|Settings|Audio", meta = (WorldContext = "WorldContextObject"))
	bool ApplyMasterVolume(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Parcel|Settings|Input")
	float GetMouseSensitivity() const { return MouseSensitivity; }

	UFUNCTION(BlueprintCallable, Category = "Parcel|Settings|Input")
	void SetMouseSensitivity(float NewMouseSensitivity);

private:
	UPROPERTY(Config)
	float MasterVolume = 1.0f;

	UPROPERTY(Config)
	float MouseSensitivity = 1.0f;

	UPROPERTY(Config)
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(Config)
	TSoftObjectPtr<USoundMix> SettingsSoundMix;

	UPROPERTY(Transient)
	TObjectPtr<USoundClass> CachedMasterSoundClass;

	UPROPERTY(Transient)
	TObjectPtr<USoundMix> CachedSettingsSoundMix;

	bool bSettingsSoundMixPushed = false;
	bool bIsValidating = false;
};
