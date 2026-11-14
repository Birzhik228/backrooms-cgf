param([string]$CaseFilter='*',[string]$Executable='')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
if(!$Executable){$Executable=Join-Path $taskRoot 'Backrooms.exe'}
$taskExe=(Resolve-Path -LiteralPath $Executable).Path
$taskRuntime=Split-Path -Parent $taskExe
if(Get-Process -Name Backrooms -ErrorAction SilentlyContinue | Where-Object {$_.Path -eq $taskExe}) {
    throw 'Close the selected Backrooms executable before verification; it shares one log file.'
}
$taskRelativeEvidence='tests/artifacts/presentation'
$taskEvidence=Join-Path $taskRuntime $taskRelativeEvidence
New-Item -ItemType Directory -Path $taskEvidence -Force | Out-Null
$taskCases=@(
    @{name='direct';mode='direct';args=@('--presentation','--guide-check','--seed','42','--parking')},
    @{name='menu-key';mode='menu-key';args=@()},
    @{name='pause-menu';mode='menu-key';args=@('--demo','--auto-pause','--width','1280','--height','720')},
    @{name='menu-click';mode='menu-click';args=@()},
    @{name='ao-on';mode='ao-on';args=@('--presentation')},
    @{name='ao-off';mode='ao-off';args=@('--presentation')},
    @{name='ao-toggle';mode='ao-toggle';args=@('--presentation')},
    @{name='camera';mode='camera';args=@('--presentation')},
    @{name='pause';mode='pause';args=@('--presentation')},
    @{name='play';mode='play';args=@('--presentation')},
    @{name='reset';mode='reset';args=@('--presentation')},
    @{name='level';mode='level';args=@('--presentation')},
    @{name='ending';mode='ending';args=@('--presentation')}
)
$taskResults=@()
foreach($taskCase in $taskCases) {
    if($taskCase.name -notlike $CaseFilter){continue}
    $taskConfig=Join-Path $taskEvidence ($taskCase.name+'.cfg')
    @('ambientOcclusion 0','brightness 0.7','blur 1.2','fov 92','headBob 0.8',
      'vsync 0','maxFps 0','musicEnabled 0') | Set-Content -LiteralPath $taskConfig -Encoding ascii
    $taskArgs=@('--hidden','--seed','12071998','--width','960','--height','640',
        '--frames','12','--fps-limit','0','--presentation-check',$taskCase.mode,
        '--settings-file',"$taskRelativeEvidence/$($taskCase.name).cfg",
        '--capture',"$taskRelativeEvidence/$($taskCase.name).bmp",
        '--profile',"$taskRelativeEvidence/$($taskCase.name).csv")+$taskCase.args
    $taskStopwatch=[Diagnostics.Stopwatch]::StartNew()
    $taskProcess=Start-Process -FilePath $taskExe -WorkingDirectory $taskRuntime -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    if(!$taskProcess.WaitForExit(120000)) {
        $taskProcess.Kill()
        throw "Timed out: $($taskCase.name)"
    }
    $taskStopwatch.Stop()
    $taskLog=Get-Content -LiteralPath (Join-Path $taskRuntime 'backrooms.log') -Raw
    Copy-Item -LiteralPath (Join-Path $taskRuntime 'backrooms.log') -Destination (Join-Path $taskEvidence ($taskCase.name+'.log')) -Force
    if($taskProcess.ExitCode -ne 0){throw "Failed $($taskCase.name): $taskLog"}
    foreach($taskExpected in @('GL errors: 0','GL resources after Game: 0',"Presentation verified: $($taskCase.mode)")) {
        if(!$taskLog.Contains($taskExpected)){throw "$($taskCase.name): missing $taskExpected`n$taskLog"}
    }
    # Opening/closing settings may rewrite formatting, but not these preferences.
    $taskPreferences=@{}
    foreach($taskLine in (Get-Content -LiteralPath $taskConfig)) {
        $taskParts=$taskLine -split '\s+'
        if($taskParts.Count -eq 2){$taskPreferences[$taskParts[0]]=[double]::Parse($taskParts[1],[Globalization.CultureInfo]::InvariantCulture)}
    }
    foreach($taskPair in @{ambientOcclusion=0;brightness=.7;blur=1.2;fov=92;headBob=.8}.GetEnumerator()) {
        if(!$taskPreferences.ContainsKey($taskPair.Key) -or [Math]::Abs($taskPreferences[$taskPair.Key]-$taskPair.Value) -gt .00001) {
            throw "$($taskCase.name): presentation persisted a change to $($taskPair.Key)"
        }
    }
    $taskResults += [pscustomobject]@{name=$taskCase.name;passed=$true;durationSeconds=[Math]::Round($taskStopwatch.Elapsed.TotalSeconds,3);args=$taskArgs}
    Write-Output "PASS $($taskCase.name)"
}
if(($taskResults.name -contains 'ao-on') -and ($taskResults.name -contains 'ao-off')) {
    $taskOn=[IO.File]::ReadAllBytes((Join-Path $taskEvidence 'ao-on.bmp'))
    $taskOff=[IO.File]::ReadAllBytes((Join-Path $taskEvidence 'ao-off.bmp'))
    $taskWidth=[BitConverter]::ToInt32($taskOn,18)
    $taskHeight=[BitConverter]::ToInt32($taskOn,22)
    if($taskWidth -ne [BitConverter]::ToInt32($taskOff,18) -or $taskHeight -ne [BitConverter]::ToInt32($taskOff,22)) {
        throw 'AO comparison images have different dimensions.'
    }
    $taskRowSize=($taskWidth*3+3)-band(-bnot 3)
    $taskOnOffset=[BitConverter]::ToInt32($taskOn,10)
    $taskOffOffset=[BitConverter]::ToInt32($taskOff,10)
    $taskChanged=0
    $taskDifference=0L
    for($taskY=[int]($taskHeight*.25);$taskY -lt [int]($taskHeight*.75);$taskY+=2) {
        for($taskX=[int]($taskWidth*.25);$taskX -lt [int]($taskWidth*.75);$taskX+=2) {
            $taskIndex=$taskY*$taskRowSize+$taskX*3
            $taskPixelDifference=0
            for($taskChannel=0;$taskChannel -lt 3;++$taskChannel) {
                $taskPixelDifference += [Math]::Abs([int]$taskOn[$taskOnOffset+$taskIndex+$taskChannel]-[int]$taskOff[$taskOffOffset+$taskIndex+$taskChannel])
            }
            if($taskPixelDifference -gt 0){++$taskChanged;$taskDifference+=$taskPixelDifference}
        }
    }
    if($taskChanged -lt 32 -or $taskDifference -lt 200){throw 'Presentation AO did not visibly change the scene outside the HUD.'}
    $taskResults += [pscustomobject]@{name='ao-scene-comparison';passed=$true;changedSampledPixels=$taskChanged;absoluteChannelDifference=$taskDifference}
    Write-Output "PASS ao-scene-comparison ($taskChanged changed sampled pixels)"
}
$taskResults | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding utf8
Write-Output "Evidence: $taskEvidence"
