param([string]$CaseFilter='*',[string]$Executable='')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
if(!$Executable){$Executable=Join-Path $taskRoot 'Backrooms.exe'}
$taskExe=(Resolve-Path -LiteralPath $Executable).Path
$taskRuntime=Split-Path -Parent $taskExe
if(Get-Process -Name Backrooms -ErrorAction SilentlyContinue | Where-Object {$_.Path -eq $taskExe}) {
    throw 'Close the selected Backrooms executable before running verification; it shares one log file.'
}
$taskRelativeEvidence='tests/artifacts/demo-controls'
$taskEvidence=Join-Path $taskRuntime $taskRelativeEvidence
New-Item -ItemType Directory -Force -Path $taskEvidence | Out-Null
@('ambientOcclusion 0.65','blur 0','headBob 0','vsync 0','maxFps 0','musicEnabled 0') |
    Set-Content -LiteralPath (Join-Path $taskEvidence 'default.cfg') -Encoding ascii
@('ambientOcclusion 0.4','blur 0','headBob 0','vsync 0','maxFps 0','musicEnabled 0') |
    Set-Content -LiteralPath (Join-Path $taskEvidence 'custom.cfg') -Encoding ascii
@('ambientOcclusion 0','blur 0','headBob 0','vsync 0','maxFps 0','musicEnabled 0') |
    Set-Content -LiteralPath (Join-Path $taskEvidence 'zero.cfg') -Encoding ascii
$taskCases=@(
    @{name='ao-on';frames=8;config='default';args=@('--demo','--no-threat');expected=@('AO amount 0.650000')},
    @{name='ao-off';frames=8;config='default';args=@('--demo','--no-threat');expected=@('AO: OFF','AO amount 0.000000')},
    @{name='ao-restore';frames=8;config='custom';args=@('--demo','--no-threat');expected=@('AO: OFF','AO: ON / amount 0.400000','AO amount 0.400000')},
    @{name='ao-zero';frames=8;config='zero';args=@('--demo','--no-threat');expected=@('AO: ON / amount 0.650000','AO amount 0.650000')},
    @{name='camera-move';frames=362;config='default';args=@('--demo','--threat-x','8','--threat-z','3.75');expected=@('camera free-fly','Inspection camera: ON')},
    @{name='camera-return';frames=440;config='default';args=@('--demo','--no-threat');expected=@('camera player','Inspection camera: OFF','Shifts: 1')},
    @{name='camera-pause';frames=18;config='default';args=@('--demo','--no-threat','--width','1280','--height','720');expected=@('camera free-fly','Paused: no')},
    @{name='camera-reset';frames=14;config='default';args=@('--demo','--no-threat');expected=@('camera player','Returned to the first room.')},
    @{name='camera-level';frames=20;config='default';args=@('--demo','--no-threat');expected=@('camera player','Level selected: Fun','Level: Fun')},
    @{name='camera-ending';frames=14;config='default';args=@('--demo','--no-threat');expected=@('camera player','Ending started: preview')},
    @{name='camera-menu';frames=8;config='default';args=@('--no-threat');expected=@('camera player','Paused: yes')},
    @{name='camera-capture';frames=9;config='default';args=@('--demo','--auto-jumpscare');expected=@('camera player','Catches: 1','Encounter: jumpscare')}
)
$taskResults=@()
foreach($taskCase in $taskCases) {
    if($taskCase.name -notlike $CaseFilter){continue}
    $taskArgs=@('--hidden','--seed','12071998','--width','960','--height','640',
        '--frames',"$($taskCase.frames)",'--fps-limit','0',
        '--demo-controls-check',$taskCase.name,
        '--settings-file',"$taskRelativeEvidence/$($taskCase.config).cfg",
        '--capture',"$taskRelativeEvidence/$($taskCase.name).bmp",
        '--profile',"$taskRelativeEvidence/$($taskCase.name).csv")+$taskCase.args
    # Every argument is a switch, number, mode or relative path without spaces.
    $taskProcess=Start-Process -FilePath $taskExe -WorkingDirectory $taskRuntime -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    if(!$taskProcess.WaitForExit(120000)) {
        $taskProcess.Kill()
        throw "Timed out: $($taskCase.name)"
    }
    $taskLog=Get-Content -LiteralPath (Join-Path $taskRuntime 'backrooms.log') -Raw
    Copy-Item -LiteralPath (Join-Path $taskRuntime 'backrooms.log') -Destination (Join-Path $taskEvidence ($taskCase.name+'.log')) -Force
    if($taskProcess.ExitCode -ne 0){throw "Failed $($taskCase.name): $taskLog"}
    foreach($taskExpected in @('GL errors: 0','GL resources after Game: 0',"Demo controls verified: $($taskCase.name)")+$taskCase.expected) {
        if(!$taskLog.Contains($taskExpected)){throw "$($taskCase.name): missing $taskExpected`n$taskLog"}
    }
    $taskResults += [pscustomobject]@{name=$taskCase.name;passed=$true;args=$taskArgs}
    Write-Output "PASS $($taskCase.name)"
}

# A hotkey log alone is insufficient: compare scene pixels outside all HUD text.
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
    if($taskChanged -lt 32 -or $taskDifference -lt 200){throw 'AO hotkey did not visibly change enough scene pixels outside the HUD.'}
    $taskResults += [pscustomobject]@{name='ao-scene-comparison';passed=$true;changedSampledPixels=$taskChanged;absoluteChannelDifference=$taskDifference}
    Write-Output "PASS ao-scene-comparison ($taskChanged changed sampled pixels)"
}
$taskResults | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding utf8
Write-Output "Evidence: $taskEvidence"
