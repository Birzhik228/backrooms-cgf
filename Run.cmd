@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" -IfNeeded
if errorlevel 1 (
  echo Build failed. See README.md.
  pause
  exit /b 1
)
start "" "%~dp0Backrooms.exe"
