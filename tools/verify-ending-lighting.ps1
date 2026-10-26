param([switch]$SkipBuild)
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $taskRoot
$taskEvidence=Join-Path $taskRoot 'tests\artifacts\ending-sun'
New-Item -ItemType Directory -Force -Path $taskEvidence | Out-Null
$taskCompiler=$null
$taskCandidates=@()
if($env:BACKROOMS_CXX){$taskCandidates+=$env:BACKROOMS_CXX}
$taskOnPath=Get-Command g++.exe -ErrorAction SilentlyContinue
if($taskOnPath){$taskCandidates+=$taskOnPath.Source}
$taskCandidates+=@('C:\msys64\mingw64\bin\g++.exe','C:\msys64\ucrt64\bin\g++.exe')
foreach($taskCandidate in $taskCandidates){
    if(Test-Path -LiteralPath $taskCandidate){
        $taskPrefix=Split-Path -Parent (Split-Path -Parent $taskCandidate)
        if(Test-Path -LiteralPath (Join-Path $taskPrefix 'include\GLFW\glfw3.h')){$taskCompiler=$taskCandidate;break}
    }
}
if(!$taskCompiler){throw 'MSYS2 MinGW GCC and GLFW are required. See README.md.'}
$env:PATH=(Split-Path -Parent $taskCompiler)+';'+$env:PATH
$taskExecutable=Join-Path $taskRoot 'tests\ending_lighting_tests.exe'
if(!$SkipBuild){
    & $taskCompiler '-std=c++17' '-O2' '-Wall' '-Wextra' '-Iinclude' '-I.' 'tests\ending_lighting_tests.cpp' 'common\Shader.cpp' 'common\glad.c' '-o' $taskExecutable '-lglfw3' '-lopengl32' '-lgdi32' '-static'
    if($LASTEXITCODE -ne 0){throw 'Ending sunlight verification compilation failed.'}
}
if(!(Test-Path -LiteralPath $taskExecutable)){throw 'Build the ending sunlight test first.'}
$taskOutput=Join-Path $taskEvidence 'lighting-tests.log'
$taskError=Join-Path $taskEvidence 'lighting-tests.stderr.log'
$taskClock=[Diagnostics.Stopwatch]::StartNew()
$taskProcess=Start-Process -FilePath $taskExecutable -WorkingDirectory $taskRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput $taskOutput -RedirectStandardError $taskError
if(!$taskProcess.WaitForExit(60000)){$taskProcess.Kill();$taskProcess.WaitForExit();throw 'Ending sunlight verification timed out.'}
$taskClock.Stop()
Get-Content -LiteralPath $taskOutput
if((Get-Item -LiteralPath $taskError).Length){Get-Content -LiteralPath $taskError}
if($taskProcess.ExitCode -ne 0){throw "Ending sunlight verification failed ($($taskProcess.ExitCode))."}
[pscustomobject]@{passed=$true;wallSeconds=[Math]::Round($taskClock.Elapsed.TotalSeconds,3);log=$taskOutput} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskEvidence 'lighting-results.json') -Encoding UTF8
