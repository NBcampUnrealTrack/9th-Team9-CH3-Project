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
    for %%D in (C D E F F G) do (
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

:MENU
cls
echo =====================================================================
echo  [Parcel Knight] Team Hamachi Multiplay Settings
echo  - Detected Engine Path: %UE_PATH%
echo =====================================================================
echo  1. Host a Server (Open Session)
echo  2. Join a Server (Enter Host's Hamachi IP)
echo  3. Exit
echo =====================================================================
set /p choice="Enter choice (1-3): "

if "%choice%"=="1" goto HOST
if "%choice%"=="2" goto CLIENT
if "%choice%"=="3" goto EXIT
goto MENU

:HOST
start "" "%UE_PATH%" "%PROJECT_PATH%" -game -windowed -ResX=1280 -ResY=720 -log -OnlineSubsystem=Null -NoSteam
exit

:CLIENT
cls
set /p IP_ADDR="Enter Host's Hamachi IP Address: "
start "" "%UE_PATH%" "%PROJECT_PATH%" %IP_ADDR% -game -windowed -ResX=1280 -ResY=720 -log -OnlineSubsystem=Null -NoSteam
exit

:EXIT
exit