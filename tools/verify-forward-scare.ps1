param([string]$Executable='Backrooms.exe',[string]$CaseFilter='*')
# Run sequentially: the executable writes one shared backrooms.log.
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskExecutable=Join-Path $taskRoot $Executable
$taskEvidenceRelative='tests/artifacts/forward-scare'
$taskEvidence=Join-Path $taskRoot $taskEvidenceRelative
New-Item -ItemType Directory -Force -Path $taskEvidence | Out-Null

function Read-Number($taskValue) {
    return [double]::Parse([string]$taskValue,[Globalization.CultureInfo]::InvariantCulture)
}
function Assert-Check([bool]$taskCondition,[string]$taskMessage) {
    if(!$taskCondition){throw $taskMessage}
}
function Wrapped-Angle([double]$taskAngle) {
    return [Math]::Atan2([Math]::Sin($taskAngle),[Math]::Cos($taskAngle))
}
function Look-Yaw($taskRow) {
    return [Math]::Atan2((Read-Number $taskRow.look_z),(Read-Number $taskRow.look_x))
}
function Shake-Energy($taskRow) {
    $taskEnergy=0.0
    foreach($taskColumn in @('shake_x','shake_y','shake_z','shake_roll')) {
        $taskValue=Read-Number $taskRow.$taskColumn
        $taskEnergy+=$taskValue*$taskValue
    }
    return $taskEnergy
}

# Use isolated fixtures; never rewrite the player's jumpscare.cfg or settings.
$taskFixtureConfig=@'
camera_turn_duration=.20
lunge_duration=.22
face_distance=.90
closeup_scale=2.2
shake_offset=.026
shake_roll=.025
fov_start=70
fov_peak=95
black_delay=.30
camera_turn=true
lunge=true
shake=true
fov_punch=true
'@
Set-Content -LiteralPath (Join-Path $taskEvidence 'default.cfg') -Value ("duration=2.0`n"+$taskFixtureConfig) -Encoding ascii
Set-Content -LiteralPath (Join-Path $taskEvidence 'three-seconds.cfg') -Value ("duration=3.0`n"+$taskFixtureConfig) -Encoding ascii
$taskSide=@('--yaw','-90','--pitch','0','--threat-x','8','--threat-z','3.75','--threat-yaw','-90','--auto-jumpscare')
$taskRear=@('--yaw','-90','--pitch','0','--threat-x','3.75','--threat-z','8','--threat-yaw','0','--auto-jumpscare')
$taskCases=@(
    @{name='side90';frames=150;args=$taskSide;config='default';expected='Encounter: game-over';kind='camera';duration=2.0},
    @{name='rear180';frames=118;args=$taskRear;config='default';expected='Encounter: jumpscare';kind='camera';duration=2.0},
    @{name='three-seconds-hold';frames=160;args=$taskSide;config='three-seconds';expected='Encounter: jumpscare';kind='duration';duration=3.0},
    @{name='three-seconds-end';frames=215;args=$taskSide;config='three-seconds';expected='Encounter: game-over';kind='duration';duration=3.0},
    @{name='walking-forward';frames=140;args=@('--yaw','0','--auto-crouch','--threat-x','28','--threat-z','3.75','--threat-yaw','90');config='default';expected='Catches: 0';kind='movement'},
    @{name='chasing-forward';frames=100;args=@('--yaw','0','--threat-x','12','--threat-z','3.75','--threat-yaw','-90');config='default';expected='Catches: 0';kind='movement'}
)
$taskResults=@()
foreach($taskCase in $taskCases) {
    if($taskCase.name -notlike $CaseFilter){continue}
    $taskStem="$taskEvidenceRelative/$($taskCase.name)"
    $taskArgs=@('--hidden','--demo','--seed','42','--frames',"$($taskCase.frames)",
        '--capture',"$taskStem.bmp",'--profile',"$taskStem.csv",'--jumpscare-config',
        "$taskEvidenceRelative/$($taskCase.config).cfg")+$taskCase.args
    $taskProcess=Start-Process -FilePath $taskExecutable -WorkingDirectory $taskRoot -ArgumentList $taskArgs -WindowStyle Hidden -Wait -PassThru
    $taskLog=Get-Content -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Raw
    Copy-Item -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Destination (Join-Path $taskEvidence ($taskCase.name+'.log'))
    Assert-Check ($taskProcess.ExitCode -eq 0 -and $taskLog.Contains('GL errors: 0') -and
        $taskLog.Contains('Loaded Aranea model: 15220 triangles.') -and $taskLog.Contains($taskCase.expected)) "Failed $($taskCase.name): $taskLog"
    $taskRows=@(Import-Csv -LiteralPath (Join-Path $taskRoot "$taskStem.csv"))
    Assert-Check ($taskRows.Count -eq $taskCase.frames) "$($taskCase.name): incomplete frame profile"
    $taskMeasurements=[ordered]@{}

    if($taskCase.kind -eq 'camera') {
        $taskScareRows=@($taskRows | Where-Object { (Read-Number $_.scare_elapsed) -gt 0 -and (Read-Number $_.scare_elapsed) -lt 1.95 })
        Assert-Check ($taskScareRows.Count -gt 100 -and $taskLog.Contains('Catches: 1')) "$($taskCase.name): expected one sustained capture"
        $taskSettled=@($taskScareRows | Where-Object { (Read-Number $_.scare_elapsed) -ge .20 -and (Read-Number $_.scare_elapsed) -le .24 })
        Assert-Check ($taskSettled.Count -gt 0) "$($taskCase.name): missing camera-settle samples"
        foreach($taskRow in $taskSettled) {
            Assert-Check ((Read-Number $taskRow.turn_weight) -ge .9999) "$($taskCase.name): turn not complete at .20 seconds"
        }
        $taskInitialYaw=Look-Yaw $taskRows[0]
        $taskFinalYaw=Look-Yaw $taskSettled[-1]
        $taskTurnAngle=[Math]::Abs((Wrapped-Angle ($taskFinalYaw-$taskInitialYaw)))
        $taskExpectedAngle=if($taskCase.name -eq 'side90'){[Math]::PI/2}else{[Math]::PI}
        Assert-Check ([Math]::Abs($taskTurnAngle-$taskExpectedAngle) -lt .08) "$($taskCase.name): actual camera did not face the side/rear creature"
        $taskPreviousError=$taskTurnAngle
        foreach($taskRow in $taskScareRows) {
            $taskTime=Read-Number $taskRow.scare_elapsed
            if($taskTime -gt .24){break}
            $taskError=[Math]::Abs((Wrapped-Angle ((Look-Yaw $taskRow)-$taskFinalYaw)))
            $taskWeight=Read-Number $taskRow.turn_weight
            Assert-Check ($taskError -le $taskPreviousError+.0001) "$($taskCase.name): camera turn reverses direction"
            Assert-Check ([Math]::Abs($taskError/$taskTurnAngle-(1-$taskWeight)) -lt .003) "$($taskCase.name): eased turn is not applied to the rendered look vector"
            if($taskTime -ge .09 -and $taskTime -le .11) {
                Assert-Check ($taskWeight -gt .83 -and $taskWeight -lt .92) "$($taskCase.name): turn lacks an ease-out midpoint"
            }
            $taskPreviousError=$taskError
        }
        $taskLungeRows=@($taskScareRows | Where-Object { (Read-Number $_.scare_elapsed) -ge .22 -and (Read-Number $_.scare_elapsed) -le .30 })
        Assert-Check ($taskLungeRows.Count -gt 0) "$($taskCase.name): missing lunge-settle samples"
        foreach($taskRow in $taskLungeRows) {
            Assert-Check ([Math]::Abs((Read-Number $taskRow.face_distance)-.90) -lt .0001) "$($taskCase.name): lunge does not settle .90 m ahead"
        }
        $taskPunchRows=@($taskScareRows | Where-Object { (Read-Number $_.scare_elapsed) -ge .10 -and (Read-Number $_.scare_elapsed) -le .14 })
        Assert-Check ($taskPunchRows.Count -gt 0) "$($taskCase.name): missing FOV punch samples"
        foreach($taskRow in $taskPunchRows) {
            $taskFov=Read-Number $taskRow.fov
            Assert-Check ($taskFov -ge 94.5 -and $taskFov -le 96.5) "$($taskCase.name): FOV fails to punch to approximately 95 degrees"
        }
        $taskEarly=@($taskScareRows | Where-Object { (Read-Number $_.scare_elapsed) -lt .50 })
        $taskLate=@($taskScareRows | Where-Object { (Read-Number $_.scare_elapsed) -gt 1.60 })
        $taskEarlyEnergy=0.0; $taskLateEnergy=0.0; $taskSignChanges=0; $taskLastShake=0.0
        foreach($taskRow in $taskEarly) {
            $taskEarlyEnergy+=Shake-Energy $taskRow
            $taskShake=Read-Number $taskRow.shake_x
            if($taskShake*$taskLastShake -lt 0){++$taskSignChanges}
            $taskLastShake=$taskShake
        }
        foreach($taskRow in $taskLate){$taskLateEnergy+=Shake-Energy $taskRow}
        $taskEarlyEnergy/=$taskEarly.Count; $taskLateEnergy/=$taskLate.Count
        Assert-Check ($taskEarlyEnergy -gt .00001 -and $taskLateEnergy -lt $taskEarlyEnergy*.02) "$($taskCase.name): random shake does not decay sufficiently"
        Assert-Check ($taskSignChanges -ge 5) "$($taskCase.name): shake lacks rapid random direction changes"
        $taskMeasurements.turnDegrees=$taskTurnAngle*180/[Math]::PI
        $taskMeasurements.lateToEarlyShakeEnergy=$taskLateEnergy/$taskEarlyEnergy
        $taskMeasurements.shakeSignChanges=$taskSignChanges
    }
    if($taskCase.kind -eq 'duration') {
        Assert-Check ($taskLog.Contains('Jumpscare: duration 3.000000 s') -and $taskLog.Contains('Catches: 1')) "$($taskCase.name): three-second configuration was not loaded"
        $taskTime=Read-Number $taskRows[-1].scare_elapsed
        if($taskCase.name -eq 'three-seconds-hold') {
            Assert-Check ($taskTime -gt 2.5 -and $taskTime -lt 2.8) 'Three-second capture ended before its configured duration'
        } else {
            Assert-Check ([Math]::Abs($taskTime-3.3) -lt .001) 'Three-second capture did not finish its black delay'
        }
        $taskMeasurements.finalScareTime=$taskTime
    }
    if($taskCase.kind -eq 'movement') {
        $taskMoved=0; $taskGaitFrames=0; $taskMinimumDot=1.0; $taskWalkingFrames=0; $taskRunningFrames=0
        for($taskIndex=1;$taskIndex -lt $taskRows.Count;++$taskIndex) {
            $taskRow=$taskRows[$taskIndex]; $taskPrevious=$taskRows[$taskIndex-1]
            $taskDx=(Read-Number $taskRow.threat_x)-(Read-Number $taskPrevious.threat_x)
            $taskDz=(Read-Number $taskRow.threat_z)-(Read-Number $taskPrevious.threat_z)
            $taskDistance=[Math]::Sqrt($taskDx*$taskDx+$taskDz*$taskDz)
            if($taskDistance -le .00001){continue}
            ++$taskMoved
            $taskYaw=Read-Number $taskRow.body_yaw
            $taskDot=($taskDx*[Math]::Sin($taskYaw)-$taskDz*[Math]::Cos($taskYaw))/$taskDistance
            $taskMinimumDot=[Math]::Min($taskMinimumDot,$taskDot)
            Assert-Check ($taskDot -gt .999 -and (Read-Number $taskRow.forward_dot) -gt .999) "$($taskCase.name): rendered body slides sideways or backwards"
            $taskSpeed=$taskDistance*60
            if($taskSpeed -gt .15) {
                ++$taskGaitFrames
                Assert-Check ([Math]::Abs((Read-Number $taskRow.head_yaw)) -lt .00001) "$($taskCase.name): moving head points sideways"
            }
            if($taskSpeed -gt 1.0 -and $taskSpeed -lt 1.2){++$taskWalkingFrames}
            if($taskSpeed -gt 3.2 -and $taskSpeed -lt 3.4){++$taskRunningFrames}
        }
        Assert-Check ($taskMoved -ge 20 -and $taskGaitFrames -ge 20) "$($taskCase.name): fixture did not exercise sustained motion"
        if($taskCase.name -eq 'walking-forward') {
            Assert-Check ($taskWalkingFrames -ge 20) 'Walking fixture did not exercise the walking gait'
        } else {
            Assert-Check ($taskRunningFrames -ge 20) 'Chasing fixture did not exercise the running gait'
        }
        $taskMeasurements.movingFrames=$taskMoved
        $taskMeasurements.minimumForwardDot=$taskMinimumDot
        $taskMeasurements.walkingFrames=$taskWalkingFrames
        $taskMeasurements.runningFrames=$taskRunningFrames
    }
    $taskResults+=[pscustomobject]@{case=$taskCase.name;args=$taskArgs;measurements=$taskMeasurements;log=$taskLog}
    Write-Output "PASS $($taskCase.name)"
}
$taskResults | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding utf8
