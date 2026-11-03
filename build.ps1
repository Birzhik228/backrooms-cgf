param(
    [switch]$Test, [switch]$DebugBuild, [switch]$GraphicsTest, [switch]$IfNeeded,
    [ValidateRange(1,32)][int]$Jobs=2
)
$ErrorActionPreference='Stop'
Set-Location -LiteralPath $PSScriptRoot
$taskCandidates=@()
if($env:BACKROOMS_CXX){$taskCandidates+=$env:BACKROOMS_CXX}
$taskOnPath=Get-Command g++.exe -ErrorAction SilentlyContinue
if($taskOnPath){$taskCandidates+=$taskOnPath.Source}
$taskCandidates+='C:\msys64\mingw64\bin\g++.exe','C:\msys64\ucrt64\bin\g++.exe'
$taskCompiler=$null
foreach($taskCandidate in $taskCandidates){
    if(Test-Path -LiteralPath $taskCandidate){
        $taskPrefix=Split-Path -Parent (Split-Path -Parent $taskCandidate)
        if(Test-Path -LiteralPath (Join-Path $taskPrefix 'include\GLFW\glfw3.h')){
            $taskCompiler=(Resolve-Path -LiteralPath $taskCandidate).Path
            break
        }
    }
}
if(!$taskCompiler){throw 'Install MSYS2 GCC and GLFW. See docs/build-and-tests.md; BACKROOMS_CXX can select g++.exe.'}
$taskBin=Split-Path -Parent $taskCompiler
$taskPrefix=Split-Path -Parent $taskBin
$env:PATH=$taskBin+';'+$env:PATH
$taskCmake=Get-Command cmake.exe -ErrorAction SilentlyContinue
if(!$taskCmake -and (Test-Path -LiteralPath 'C:\Program Files\CMake\bin\cmake.exe')){
    $taskCmake=Get-Item -LiteralPath 'C:\Program Files\CMake\bin\cmake.exe'
}
if(!$taskCmake){throw 'Install CMake 3.21 or newer. See docs/build-and-tests.md.'}
$taskCmakePath=if($taskCmake -is [System.Management.Automation.CommandInfo]){$taskCmake.Source}else{$taskCmake.FullName}
$taskCtestPath=Join-Path (Split-Path -Parent $taskCmakePath) 'ctest.exe'
$taskMake=Join-Path $taskBin 'mingw32-make.exe'
if(!(Test-Path -LiteralPath $taskMake)){throw 'Install the MSYS2 MinGW make package. See docs/build-and-tests.md.'}
$taskConfig=if($DebugBuild){'Debug'}else{'Release'}
$taskBuild=Join-Path $PSScriptRoot ('build\windows-'+$taskConfig.ToLowerInvariant())
$taskOutput=Join-Path $PSScriptRoot 'Backrooms.exe'
$taskRunning=Get-Process -Name Backrooms -ErrorAction SilentlyContinue | Where-Object {$_.Path -eq $taskOutput}
if($taskRunning){throw 'Close the Backrooms window before updating this executable.'}
if($IfNeeded){Write-Host 'Checking the CMake dependency graph; unchanged source is not recompiled.'}
$taskGraphics=if($GraphicsTest){'ON'}else{'OFF'}
& $taskCmakePath -S $PSScriptRoot -B $taskBuild -G 'MinGW Makefiles' `
    "-DCMAKE_CXX_COMPILER=$taskCompiler" "-DCMAKE_C_COMPILER=$(Join-Path $taskBin 'gcc.exe')" `
    "-DCMAKE_MAKE_PROGRAM=$taskMake" "-DCMAKE_PREFIX_PATH=$taskPrefix" `
    "-DCMAKE_BUILD_TYPE=$taskConfig" '-DBUILD_TESTING=ON' "-DBACKROOMS_GRAPHICS_TESTS=$taskGraphics"
if($LASTEXITCODE -ne 0){throw 'CMake configuration failed.'}
$taskTargets=@('backrooms_deploy')
if($Test){$taskTargets+='native_tests'}
if($GraphicsTest){$taskTargets+='graphics_tests_build'}
& $taskCmakePath --build $taskBuild --config $taskConfig --parallel $Jobs --target @taskTargets
if($LASTEXITCODE -ne 0){throw 'CMake build failed.'}
if($Test){
    & $taskCtestPath --test-dir $taskBuild -C $taskConfig -L '^native$' --output-on-failure
    if($LASTEXITCODE -ne 0){throw 'Native tests failed.'}
}
if($GraphicsTest){
    & $taskCtestPath --test-dir $taskBuild -C $taskConfig -L '^graphics$' --output-on-failure
    if($LASTEXITCODE -ne 0){throw 'Graphics tests failed (a working OpenGL 3.3 context is required).'}
}
Write-Host 'Backrooms.exe is ready. Start with Run.cmd or open the executable.'
