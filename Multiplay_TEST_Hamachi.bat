@echo off
cd /d "%~dp0"

set "UE_PATH=C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT_PATH=%~dp0Parcel_Knight.uproject"

:MENU
cls
echo ===================================================
echo   [Parcel Knight] 팀 하마치 멀티플레이어 테스트 세팅
echo ===================================================
echo  1. 내가 방장(호스트)으로 서버 열기
echo  2. 방장 하마치 IP 주소 입력해서 참가하기
echo  3. 종료
echo ===================================================
set /p choice="번호 입력 (1-3): "

if "%choice%"=="1" goto HOST
if "%choice%"=="2" goto CLIENT
if "%choice%"=="3" goto EXIT
goto MENU

:HOST
:: ───> 맨 끝에 스팀 우회용 LAN 모드 인자 추가
start "" "%UE_PATH%" "%PROJECT_PATH%" -game -windowed -ResX=1280 -ResY=720 -log -OnlineSubsystem=Null -NoSteam
exit

:CLIENT
cls
set /p IP_ADDR="접속할 방장의 하마치 IP를 입력하세요: "
:: ───> 맨 끝에 스팀 우회용 LAN 모드 인자 추가
start "" "%UE_PATH%" "%PROJECT_PATH%" %IP_ADDR% -game -windowed -ResX=1280 -ResY=720 -log -OnlineSubsystem=Null -NoSteam
exit

:EXIT
exit