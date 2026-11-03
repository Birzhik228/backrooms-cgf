param([switch]$SkipBuild)
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $taskRoot
$taskEvidence=Join-Path $taskRoot 'tests\artifacts\ending-sun'
New-Item -ItemType Directory -Force -Path $taskEvidence | Out-Null
$taskExecutable=Join-Path $taskRoot 'build\windows-release\bin\ending_lighting_tests.exe'
if(!$SkipBuild){
    # Compilation, dependencies and test registration belong to CMake. The
    # dedicated run below preserves this helper's evidence files and logs.
    & (Join-Path $taskRoot 'build.ps1') -GraphicsTest
}
if(!(Test-Path -LiteralPath $taskExecutable)){throw 'Run build.ps1 -GraphicsTest before using -SkipBuild.'}
$taskOutput=Join-Path $taskEvidence 'lighting-tests.log'
$taskError=Join-Path $taskEvidence 'lighting-tests.stderr.log'
$taskClock=[Diagnostics.Stopwatch]::StartNew()
$taskProcess=Start-Process -FilePath $taskExecutable -WorkingDirectory $taskRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput $taskOutput -RedirectStandardError $taskError
# Keep the handle while the child runs so Windows PowerShell can read its exit
# code after the process has already left the process table.
$taskProcessHandle=$taskProcess.Handle
if(!$taskProcess.WaitForExit(60000)){$taskProcess.Kill();$taskProcess.WaitForExit();throw 'Ending sunlight verification timed out.'}
$taskClock.Stop()
Get-Content -LiteralPath $taskOutput
if((Get-Item -LiteralPath $taskError).Length){Get-Content -LiteralPath $taskError}
if($taskProcess.ExitCode -ne 0){throw "Ending sunlight verification failed ($($taskProcess.ExitCode))."}
[pscustomobject]@{passed=$true;wallSeconds=[Math]::Round($taskClock.Elapsed.TotalSeconds,3);log=$taskOutput} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskEvidence 'lighting-results.json') -Encoding UTF8
