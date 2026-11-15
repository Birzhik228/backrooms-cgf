@echo off
setlocal
cd /d "%~dp0"
echo Checking the extracted submission package...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\verify-submission.ps1"
if errorlevel 1 goto failed
echo Measuring AO performance at 1280x720. Please keep other demanding applications closed.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\benchmark-ao.ps1"
if errorlevel 1 goto failed
echo.
echo Checks finished. Results are under tests\artifacts\submission and tests\artifacts\ao-performance.
echo Keep these results with the computer's details for the report.
pause
exit /b 0
:failed
echo.
echo A check failed. Keep the error text and logs in tests\artifacts for diagnosis.
pause
exit /b 1
