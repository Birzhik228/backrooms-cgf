param([string]$CaseFilter='*')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskEvidence=Join-Path $taskRoot 'tests\artifacts\aranea'
New-Item -ItemType Directory -Force $taskEvidence | Out-Null
$taskPose=@('--threat-preview','--threat-x','8','--threat-z','3.75','--threat-yaw','-90','--yaw','0','--pitch','2','--camera-view')
$taskCases=@(
 @{name='walk-support';frames=15;args=$taskPose+@('--threat-time','0.12','--threat-speed','1.1');expected='GL errors: 0'},
 @{name='walk-transfer';frames=15;args=$taskPose+@('--threat-time','0.43','--threat-speed','1.1');expected='GL errors: 0'},
 @{name='run-support';frames=15;args=$taskPose+@('--threat-time','0.08','--threat-speed','3.3');expected='GL errors: 0'},
 @{name='run-reach';frames=15;args=$taskPose+@('--threat-time','0.38','--threat-speed','3.3');expected='GL errors: 0'},
 @{name='head-left';frames=15;args=$taskPose+@('--threat-head-yaw','-45','--threat-head-pitch','-8');expected='GL errors: 0'},
 @{name='head-right';frames=15;args=$taskPose+@('--threat-head-yaw','45');expected='GL errors: 0'},
 @{name='stoop';frames=15;args=@('--x','-3.75','--z','3.75','--yaw','-90','--threat-preview','--threat-x','-3.75','--threat-z','.95','--threat-yaw','180','--threat-speed','1.1','--threat-time','.3');expected='GL errors: 0'},
 @{name='wall-fold';frames=30;args=@('--x','-11.5','--z','7.5','--yaw','0','--threat-preview','--threat-x','-8.5','--threat-z','7.5','--threat-yaw','-90','--threat-speed','1.1','--threat-time','.3');expected='GL errors: 0'},
 # Allow time for the doorway pivot before translation, then the full scare.
 @{name='door-chase';frames=300;args=@('--x','-3.75','--z','1','--yaw','-90','--threat-x','-3.75','--threat-z','-4','--threat-yaw','180','--auto-door');expected='Encounter: game-over'},
 @{name='walking';frames=440;args=@('--auto-crouch','--yaw','0','--threat-x','28','--threat-z','3.75','--threat-yaw','90');expected='Creature footfalls:'},
 @{name='chasing';frames=100;args=@('--yaw','0','--threat-x','12','--threat-z','3.75','--threat-yaw','-90');expected='Creature footfalls:'},
 @{name='pause';frames=100;args=@('--threat-x','8','--threat-z','3.75','--threat-yaw','-90','--auto-pause');expected='Paused: yes'},
 @{name='capture';frames=75;args=@('--auto-catch');expected='Encounter: jumpscare'},
 @{name='restart';frames=180;args=@('--auto-catch','--auto-restart');expected='Restarts: 1'},
 @{name='odd-size';frames=15;args=$taskPose+@('--threat-speed','1.1','--threat-time','.43','--width','1919','--height','1027');expected='GL errors: 0'}
)
$taskResults=@()
foreach($taskCase in $taskCases) {
 if($taskCase.name -notlike $CaseFilter){continue}
 $taskArgs=@('--hidden','--demo','--frames',"$($taskCase.frames)",'--capture',"tests/artifacts/aranea/$($taskCase.name).bmp")+$taskCase.args
 $taskProcess=Start-Process -FilePath (Join-Path $taskRoot 'Backrooms.exe') -WorkingDirectory $taskRoot -ArgumentList $taskArgs -WindowStyle Hidden -Wait -PassThru
 $taskLog=[string](Get-Content -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Raw)
 Copy-Item -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Destination (Join-Path $taskEvidence ($taskCase.name+'.log'))
 if($taskProcess.ExitCode -ne 0 -or !$taskLog.Contains('GL errors: 0') -or !$taskLog.Contains($taskCase.expected) -or !$taskLog.Contains('Loaded Aranea model: 15220 triangles.')){throw "Failed $($taskCase.name): $taskLog"}
 if($taskCase.name -in @('walking','chasing')) {
  if($taskLog -notmatch 'Creature footfalls: ([0-9]+)' -or [int]$Matches[1] -lt 2){throw "No repeated foot plants in $($taskCase.name): $taskLog"}
 }
 if($taskCase.name -eq 'pause' -and $taskLog -notmatch 'Creature footfalls: 0'){throw 'Paused creature emitted steps'}
 $taskResults += [pscustomobject]@{case=$taskCase.name;args=$taskArgs;log=$taskLog}
 Write-Output "PASS $($taskCase.name)"
}
$taskResults | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding utf8
