@echo off
cd /d "%~dp0"

:: =====================================================================
:: [Smart Search] Auto-detect Unreal Editor 5.5 path via Registry
:: =====================================================================
set "UE_PATH="

for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.5" /v "InstalledDirectory" 2^>nul') do (
    set "UE_PATH=%%B\Engine\Binaries\Win64\UnrealEditor.exe"
)
if not exist "%UE_PATH%" (
    for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\WOW6432Node\EpicGames\Unreal Engine\5.5" /v "InstalledDirectory" 2^>nul') do (
        set "UE_PATH=%%B\Engine\Binaries\Win64\UnrealEditor.exe"
    )
)

:: Drive Search Loop Fallback (C to G)
if not exist "%UE_PATH%" (
    for %%D in (C D E F G) do (
        if not exist "%UE_PATH%" (
            if exist "%%D:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe" (
                set "UE_PATH=%%D:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe"
            ) else if exist "%%D:\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe" (
                set "UE_PATH=%%D:\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe"
            ) else if exist "%%D:\Unreal\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe" (
                set "UE_PATH=%%D:\Unreal\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe"
            )
        )
    )
)

:: Verification
if not exist "%UE_PATH%" (
    echo =====================================================================
    echo  [ERROR] Unreal Editor 5.5 execution file not found!
    echo =====================================================================
    pause
    exit
)

set "PROJECT_PATH=%~dp0Parcel_Knight.uproject"
if not exist "%PROJECT_PATH%" (
    echo =====================================================================
    echo  [ERROR] .uproject file not found!
    echo =====================================================================
    pause
    exit
)

echo =====================================================================
echo  [Parcel Knight] Steam OSS Multiplay Test
echo  - Detected Engine Path: %UE_PATH%
echo  - NOTICE: Please ensure the Steam Client is running before launch!
echo =====================================================================

start "" "%UE_PATH%" "%PROJECT_PATH%" -game -windowed -ResX=1280 -ResY=720 -DefSubsystem=Steam -log
exit