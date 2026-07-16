@echo off
cd /d "%~dp0"

set "UE_PATH=C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT_PATH=%~dp0Parcel_Knight.uproject"

echo ===================================================
echo   [Parcel Knight] 스팀 API 연동 멀티플레이 테스트
echo ===================================================

start "" "%UE_PATH%" "%PROJECT_PATH%" -game -windowed -ResX=1280 -ResY=720 -DefSubsystem=Steam -log
exit