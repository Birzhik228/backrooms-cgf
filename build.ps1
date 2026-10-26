param([switch]$Test, [switch]$DebugBuild, [switch]$GraphicsTest, [switch]$IfNeeded)
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$taskOutputPath = Join-Path $PSScriptRoot 'Backrooms.exe'
if ($IfNeeded -and !$Test -and !$GraphicsTest -and !$DebugBuild -and (Test-Path -LiteralPath $taskOutputPath)) {
    $taskBuildTime = (Get-Item -LiteralPath $taskOutputPath).LastWriteTimeUtc
    $taskInputs = @(Get-Item -LiteralPath (Join-Path $PSScriptRoot 'backrooms.cpp'), (Join-Path $PSScriptRoot 'world.cpp'), (Join-Path $PSScriptRoot 'world.h'), $PSCommandPath)
    $taskInputs += Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'common') -File -Recurse | Where-Object { $_.Extension -in '.cpp','.c','.h' }
    if (!($taskInputs | Where-Object { $_.LastWriteTimeUtc -gt $taskBuildTime })) {
        Write-Host 'Backrooms.exe is up to date.'
        exit 0
    }
    Write-Host 'Source files changed. Rebuilding Backrooms.exe before launch.'
}
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
$taskSources = @('backrooms.cpp','world.cpp','common\threat.cpp','common\threat_model.cpp','common\audio.cpp','common\music.cpp','common\Shader.cpp','common\ui.cpp','common\textures.cpp','common\glad.c')
Write-Host "Building Backrooms with $taskCompiler"
& $taskCompiler @taskFlags @taskSources '-o' 'Backrooms.exe' '-lglfw3' '-lopengl32' '-lgdi32' '-lwinmm' '-static' '-mwindows'
if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)." }
Write-Host 'Built Backrooms.exe. Start with Run.cmd.'
if ($Test) {
    & $taskCompiler @taskFlags 'world.cpp' 'tests\atmosphere_tests.cpp' '-o' 'tests\atmosphere_tests.exe' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Atmosphere test compilation failed.' }
    & '.\tests\atmosphere_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Atmosphere tests failed.' }
    & $taskCompiler @taskFlags 'world.cpp' 'tests\parking_tests.cpp' '-o' 'tests\parking_tests.exe' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Parking test compilation failed.' }
    & '.\tests\parking_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Parking tests failed.' }
    & $taskCompiler @taskFlags 'world.cpp' 'tests\partygoer_tests.cpp' '-o' 'tests\partygoer_tests.exe' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Partygoer test compilation failed.' }
    & '.\tests\partygoer_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Partygoer tests failed.' }

    & $taskCompiler @taskFlags 'world.cpp' 'tests\fun_tests.cpp' '-o' 'tests\fun_tests.exe' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Level Fun test compilation failed.' }
    & '.\tests\fun_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Level Fun tests failed.' }
    & $taskCompiler @taskFlags 'world.cpp' 'common\threat.cpp' 'tests\progression_tests.cpp' '-o' 'tests\progression_tests.exe' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Progression test compilation failed.' }
    & '.\tests\progression_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Progression tests failed.' }
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
    & $taskCompiler @taskFlags 'world.cpp' 'common\threat.cpp' 'tests\threat_tests.cpp' '-o' 'tests\threat_tests.exe' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Threat test compilation failed.' }
    & '.\tests\threat_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Threat tests failed.' }
    & $taskCompiler @taskFlags 'common\threat_model.cpp' 'tests\threat_model_tests.cpp' '-o' 'tests\threat_model_tests.exe' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Threat model test compilation failed.' }
    & '.\tests\threat_model_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Threat model tests failed.' }
    & $taskCompiler @taskFlags 'tests\encounter_tests.cpp' '-o' 'tests\encounter_tests.exe' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Encounter test compilation failed.' }
    & '.\tests\encounter_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Encounter tests failed.' }
    & $taskCompiler @taskFlags 'common\audio.cpp' 'tests\audio_tests.cpp' '-o' 'tests\audio_tests.exe' '-lwinmm' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Audio test compilation failed.' }
    & '.\tests\audio_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Audio tests failed.' }
    & $taskCompiler @taskFlags 'common\music.cpp' 'tests\music_tests.cpp' '-o' 'tests\music_tests.exe' '-lwinmm' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Music test compilation failed.' }
    & '.\tests\music_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Music tests failed.' }
}

if ($GraphicsTest) {
    & $taskCompiler @taskFlags 'tests\graphics_tests.cpp' 'common\Shader.cpp' 'common\glad.c' '-o' 'tests\graphics_tests.exe' '-lglfw3' '-lopengl32' '-lgdi32' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Graphics test compilation failed.' }
    & '.\tests\graphics_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Graphics tests failed.' }
    & $taskCompiler @taskFlags 'tests\lens_noise_tests.cpp' 'common\Shader.cpp' 'common\glad.c' '-o' 'tests\lens_noise_tests.exe' '-lglfw3' '-lopengl32' '-lgdi32' '-static'
    if ($LASTEXITCODE -ne 0) { throw 'Lens noise test compilation failed.' }
    & '.\tests\lens_noise_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Lens noise tests failed.' }
}
