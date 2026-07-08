@echo off
:: 첫 번째 클라이언트 실행 (호스트용)
start "" "C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0..\..\Parcel_Knight.uproject" -game -windowed -ResX=960 -ResY=540 -log

:: 1초 지연 후 두 번째 클라이언트 실행 (참가자용)
timeout /t 1 /nobreak >nul
start "" "C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0..\..\Parcel_Knight.uproject" -game -windowed -ResX=960 -ResY=540 -log