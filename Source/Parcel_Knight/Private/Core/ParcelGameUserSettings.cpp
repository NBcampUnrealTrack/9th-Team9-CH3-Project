#include "Core/ParcelGameUserSettings.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

DEFINE_LOG_CATEGORY_STATIC(LogParcelGameUserSettings, Log, All);

namespace ParcelSettingsDefaults
{
	constexpr float MasterVolume = 1.0f;
	constexpr float MouseSensitivity = 1.0f;
	constexpr float MinMouseSensitivity = 0.1f;
	constexpr float MaxMouseSensitivity = 3.0f;

	const FSoftObjectPath MasterSoundClassPath(
		TEXT("/Game/Audio/Settings/SC_Parcel_Master.SC_Parcel_Master"));
	const FSoftObjectPath SettingsSoundMixPath(
		TEXT("/Game/Audio/Settings/SM_Parcel_Settings.SM_Parcel_Settings"));
}

UParcelGameUserSettings::UParcelGameUserSettings()
	: MasterSoundClass(ParcelSettingsDefaults::MasterSoundClassPath)
	, SettingsSoundMix(ParcelSettingsDefaults::SettingsSoundMixPath)
{
}

void UParcelGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	MasterVolume = ParcelSettingsDefaults::MasterVolume;
	MouseSensitivity = ParcelSettingsDefaults::MouseSensitivity;
}

void UParcelGameUserSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);
	ValidateSettings();
}

void UParcelGameUserSettings::ValidateSettings()
{
	Super::ValidateSettings();

	MasterVolume = FMath::Clamp(MasterVolume, 0.0f, 1.0f);
	MouseSensitivity = FMath::Clamp(
		MouseSensitivity,
		ParcelSettingsDefaults::MinMouseSensitivity,
		ParcelSettingsDefaults::MaxMouseSensitivity);

	if (MasterSoundClass.IsNull())
	{
		MasterSoundClass = TSoftObjectPtr<USoundClass>(ParcelSettingsDefaults::MasterSoundClassPath);
	}

	if (SettingsSoundMix.IsNull())
	{
		SettingsSoundMix = TSoftObjectPtr<USoundMix>(ParcelSettingsDefaults::SettingsSoundMixPath);
	}
}

UParcelGameUserSettings* UParcelGameUserSettings::GetParcelGameUserSettings()
{
	return Cast<UParcelGameUserSettings>(UGameUserSettings::GetGameUserSettings());
}

void UParcelGameUserSettings::SetMasterVolume(float NewMasterVolume)
{
	MasterVolume = FMath::Clamp(NewMasterVolume, 0.0f, 1.0f);
}

bool UParcelGameUserSettings::ApplyMasterVolume(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	if (!CachedMasterSoundClass)
	{
		CachedMasterSoundClass = MasterSoundClass.LoadSynchronous();
	}

	if (!CachedSettingsSoundMix)
	{
		CachedSettingsSoundMix = SettingsSoundMix.LoadSynchronous();
	}

	if (!CachedMasterSoundClass || !CachedSettingsSoundMix)
	{
		UE_LOG(
			LogParcelGameUserSettings,
			Warning,
			TEXT("Master volume could not be applied. SoundClass=%s Valid=%d SoundMix=%s Valid=%d"),
			*MasterSoundClass.ToSoftObjectPath().ToString(),
			IsValid(CachedMasterSoundClass),
			*SettingsSoundMix.ToSoftObjectPath().ToString(),
			IsValid(CachedSettingsSoundMix));
		return false;
	}

	if (!bSettingsSoundMixPushed)
	{
		UGameplayStatics::PushSoundMixModifier(WorldContextObject, CachedSettingsSoundMix);
		bSettingsSoundMixPushed = true;
	}

	UGameplayStatics::SetSoundMixClassOverride(
		WorldContextObject,
		CachedSettingsSoundMix,
		CachedMasterSoundClass,
		MasterVolume,
		1.0f,
		0.0f,
		true);

	UE_LOG(LogParcelGameUserSettings, Log, TEXT("Applied local MasterVolume=%.2f"), MasterVolume);
	return true;
}

void UParcelGameUserSettings::SetMouseSensitivity(float NewMouseSensitivity)
{
	MouseSensitivity = FMath::Clamp(
		NewMouseSensitivity,
		ParcelSettingsDefaults::MinMouseSensitivity,
		ParcelSettingsDefaults::MaxMouseSensitivity);
}
