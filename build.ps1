param([switch]$Test, [switch]$DebugBuild)
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$taskOutputPath = Join-Path $PSScriptRoot 'Backrooms.exe'
$taskRunningGame = Get-Process -Name Backrooms -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $taskOutputPath }
if ($taskRunningGame) { throw 'Close the Backrooms window before rebuilding this executable.' }
$taskCompiler = $null
$taskCandidatePaths = @()
if ($env:BACKROOMS_CXX) { $taskCandidatePaths += $env:BACKROOMS_CXX }
$taskOnPath = Get-Command g++.exe -ErrorAction SilentlyContinue
if ($taskOnPath) { $taskCandidatePaths += $taskOnPath.Source }
$taskCandidatePaths += 'C:\msys64\mingw64\bin\g++.exe', 'C:\msys64\ucrt64\bin\g++.exe'
foreach ($taskCandidate in $taskCandidatePaths) {
    if (Test-Path -LiteralPath $taskCandidate) {
        $taskPrefix = Split-Path -Parent (Split-Path -Parent $taskCandidate)
        if (Test-Path -LiteralPath (Join-Path $taskPrefix 'include\GLFW\glfw3.h')) { $taskCompiler = $taskCandidate; break }
    }
}
if (-not $taskCompiler) { throw 'Install MSYS2 MinGW64 GCC and GLFW first. See README.md. You can also set BACKROOMS_CXX to g++.exe.' }
$env:PATH = (Split-Path -Parent $taskCompiler) + ';' + $env:PATH
$taskFlags = @('-std=c++17','-Wall','-Wextra','-Iinclude','-I.')
if ($DebugBuild) { $taskFlags += @('-O0','-g') } else { $taskFlags += '-O2' }
$taskSources = @('backrooms.cpp','world.cpp','common\audio.cpp','common\music.cpp','common\Shader.cpp','common\ui.cpp','common\textures.cpp','common\glad.c')
Write-Host "Building Backrooms with $taskCompiler"
& $taskCompiler @taskFlags @taskSources '-o' 'Backrooms.exe' '-lglfw3' '-lopengl32' '-lgdi32' '-lwinmm' '-static' '-mwindows'
if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)." }
Write-Host 'Built Backrooms.exe. Start with Run.cmd.'
if ($Test) {
    & $taskCompiler @taskFlags 'world.cpp' 'tests\world_tests.cpp' '-o' 'tests\world_tests.exe' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'World test compilation failed.' }
    & '.\tests\world_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'World tests failed.' }
    & $taskCompiler @taskFlags 'world.cpp' 'tests\streaming_tests.cpp' '-o' 'tests\streaming_tests.exe' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Streaming test compilation failed.' }
    & '.\tests\streaming_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Streaming tests failed.' }
    & $taskCompiler @taskFlags 'tests\movement_tests.cpp' '-o' 'tests\movement_tests.exe' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Movement test compilation failed.' }
    & '.\tests\movement_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Movement tests failed.' }
    & $taskCompiler @taskFlags 'common\audio.cpp' 'tests\audio_tests.cpp' '-o' 'tests\audio_tests.exe' '-lwinmm' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Audio test compilation failed.' }
    & '.\tests\audio_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Audio tests failed.' }
    & $taskCompiler @taskFlags 'common\music.cpp' 'tests\music_tests.cpp' '-o' 'tests\music_tests.exe' '-lwinmm' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Music test compilation failed.' }
    & '.\tests\music_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Music tests failed.' }
}
