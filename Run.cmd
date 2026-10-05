@echo off
cd /d "%~dp0"
if not exist "%~dp0Backrooms.exe" (
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1"
  if errorlevel 1 (
    echo Build failed. See README.md.
    pause
    exit /b 1
  )
)
start "" "%~dp0Backrooms.exe"
