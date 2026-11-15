@echo off
cd /d "%~dp0"
if exist "%~dp0Backrooms.exe" goto launch
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" -IfNeeded
if errorlevel 1 (
  echo Build failed. See README.md.
  pause
  exit /b 1
)
:launch
start "" "%~dp0Backrooms.exe" %*
