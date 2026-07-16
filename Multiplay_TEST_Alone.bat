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

:: Path Verification
if not exist "%UE_PATH%" (
    echo =====================================================================
    echo  [ERROR] Unreal Editor 5.5 execution file not found!
    echo  Please open this .bat file and set your custom UE_PATH manually.
    echo =====================================================================
    pause
    exit
)

set "PROJECT_PATH=%~dp0Parcel_Knight.uproject"
if not exist "%PROJECT_PATH%" (
    echo =====================================================================
    echo  [ERROR] .uproject file not found!
    echo  This batch file must be located in the project root directory.
    echo =====================================================================
    pause
    exit
)

echo =====================================================================
echo  [Parcel Knight] Local LAN Multiplay Test (2 Windows)
echo  - Detected Engine Path: %UE_PATH%
echo =====================================================================

:: 1) Launch Listen Server
start "" "%UE_PATH%" "%PROJECT_PATH%" -game -windowed -ResX=960 -ResY=540 -log -OnlineSubsystem=Null -NoSteam

:: 2) Launch Client
timeout /t 1 >nul
start "" "%UE_PATH%" "%PROJECT_PATH%" 127.0.0.1 -game -windowed -ResX=960 -ResY=540 -log -OnlineSubsystem=Null -NoSteam
exit