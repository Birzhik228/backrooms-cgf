@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" -Test
if errorlevel 1 (
  echo Build failed. Read the message above and README.md.
  pause
  exit /b 1
)
pause
