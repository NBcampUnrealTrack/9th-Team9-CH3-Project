@echo off
cd /d "%~dp0"
set "UE_PATH=C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT_PATH=%~dp0Parcel_Knight.uproject"

echo ===================================================
echo [Multiplay TEST] 언리얼 멀티플레이어 테스트를 시작합니다.
echo 스팀 서브시스템을 우회하여 로컬 LAN 모드로 실행합니다.
echo 프로젝트 경로: %PROJECT_PATH%
echo ===================================================

:: 1) 리슨 서버 (Listen Server) 실행: 맨 끝에 -OnlineSubsystem=Null 추가
start "" "%UE_PATH%" "%PROJECT_PATH%" -game -windowed -ResX=960 -ResY=540 -log -OnlineSubsystem=Null -NoSteam

:: 2) 클라이언트 (Client) 실행: 맨 끝에 -OnlineSubsystem=Null 추가
timeout /t 1 >nul
start "" "%UE_PATH%" "%PROJECT_PATH%" 127.0.0.1 -game -windowed -ResX=960 -ResY=540 -log -OnlineSubsystem=Null -NoSteam