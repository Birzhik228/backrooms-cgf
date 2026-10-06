param([string]$Executable='Backrooms.exe',[string]$CaseFilter='')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskExecutable=Join-Path $taskRoot $Executable
if(!(Test-Path -LiteralPath $taskExecutable)){throw "Build the game first: $taskExecutable"}
$taskArtifacts=Join-Path $taskRoot 'tests\artifacts\v12'
New-Item -ItemType Directory -Path $taskArtifacts -Force | Out-Null
$taskCases=@(
 @{name='main-menu';frames=12;args=@();expect='Paused: yes'},
 @{name='creature-front';frames=20;args=@('--demo','--yaw','0','--pitch','3','--threat-preview','--threat-x','8','--threat-z','3.75','--threat-yaw','-90','--camera-view');expect='Threat: WANDER'},
 @{name='creature-dark';frames=20;args=@('--demo','--yaw','0','--pitch','3','--threat-preview','--threat-x','10','--threat-z','3.75','--threat-yaw','-90','--no-flashlight','--camera-view');expect='Threat: WANDER'},
 @{name='creature-stoop';frames=20;args=@('--demo','--x','-3.75','--z','3.75','--yaw','-90','--threat-preview','--threat-x','-3.75','--threat-z','0.95','--threat-yaw','180','--camera-view');expect='Threat: WANDER'},
 @{name='creature-door-chase';frames=240;args=@('--demo','--x','-3.75','--z','1','--yaw','-90','--threat-x','-3.75','--threat-z','-4','--threat-yaw','180','--auto-door');expect='Catches: 1. Restarts: 0. Encounter: game-over'},
 @{name='walk-pose-a';frames=20;args=@('--demo','--yaw','0','--threat-preview','--threat-x','8','--threat-z','3.75','--threat-yaw','-90','--threat-time','0.22','--threat-speed','1.1','--camera-view');expect='Threat: WANDER'},
 @{name='walk-pose-b';frames=20;args=@('--demo','--yaw','0','--threat-preview','--threat-x','8','--threat-z','3.75','--threat-yaw','-90','--threat-time','0.72','--threat-speed','1.1','--camera-view');expect='Threat: WANDER'},
 @{name='run-pose-a';frames=20;args=@('--demo','--yaw','0','--threat-preview','--threat-x','8','--threat-z','3.75','--threat-yaw','-90','--threat-time','0.12','--threat-speed','3.3','--camera-view');expect='Threat: WANDER'},
 @{name='run-pose-b';frames=20;args=@('--demo','--yaw','0','--threat-preview','--threat-x','8','--threat-z','3.75','--threat-yaw','-90','--threat-time','0.42','--threat-speed','3.3','--camera-view');expect='Threat: WANDER'},
 @{name='jumpscare-late';frames=61;args=@('--demo','--auto-catch','--no-flashlight');expect='Encounter: jumpscare'},
 @{name='chase';frames=35;args=@('--demo','--yaw','0','--threat-x','8','--threat-z','3.75','--threat-yaw','-90','--metrics');expect='Threat: CHASING'},
 @{name='chase-pause';frames=100;args=@('--demo','--threat-x','8','--threat-z','3.75','--threat-yaw','-90','--auto-pause');expect='Catches: 0. Restarts: 0. Encounter: exploring'},
 @{name='jumpscare';frames=25;args=@('--demo','--auto-catch');expect='Catches: 1. Restarts: 0. Encounter: jumpscare'},
 @{name='jumpscare-crouch';frames=35;args=@('--demo','--auto-catch','--auto-crouch','--pitch','-75');expect='Encounter: jumpscare'},
 @{name='game-over';frames=90;args=@('--demo','--auto-catch');expect='Catches: 1. Restarts: 0. Encounter: game-over'},
 @{name='caught-by-pursuit';frames=180;args=@('--demo','--yaw','0','--threat-x','8','--threat-z','3.75','--threat-yaw','-90');expect='Catches: 1. Restarts: 0. Encounter: game-over'},
 @{name='restart-key';frames=130;args=@('--demo','--auto-catch','--auto-restart');expect='Catches: 1. Restarts: 1. Encounter: exploring'},
 @{name='restart-click';frames=130;args=@('--demo','--auto-catch','--auto-restart-click','--width','960','--height','640');expect='Catches: 1. Restarts: 1. Encounter: exploring'},
 @{name='escape-cannot-resume';frames=90;args=@('--demo','--auto-catch','--auto-scare-escape');expect='Catches: 1. Restarts: 0. Encounter: game-over'},
 @{name='settings';frames=20;args=@('--settings','--width','960','--height','640');expect='Music enabled: yes'},
 @{name='exit-traverse';frames=140;args=@('--demo','--x','6.225','--z','-265.8','--yaw','-90','--auto-exit','--auto-walk');expect='Exit used: yes. Escaped: yes'},
 @{name='exit-continue';frames=200;args=@('--demo','--x','6.225','--z','-265.8','--yaw','-90','--auto-exit','--auto-walk','--auto-continue');expect='Exit used: yes. Escaped: no'},
 @{name='natural-spawn';frames=740;args=@('--demo','--auto-crouch','--metrics');expect='Threat: WANDER'},
 @{name='chase-profile';frames=600;args=@('--demo','--yaw','0','--auto-walk','--auto-sprint','--threat-x','-12','--threat-z','3.75','--threat-yaw','90','--profile','tests/artifacts/v12/chase.csv');expect='GL errors: 0'},
 @{name='streaming';frames=3000;args=@('--demo','--yaw','0','--auto-walk','--auto-sprint','--no-threat','--profile','tests/artifacts/v12/streaming.csv');expect='GL errors: 0'}
)
$taskResults=foreach($taskCase in $taskCases){
 if($CaseFilter -and $taskCase.name -notmatch $CaseFilter){continue}
 $taskArguments=@('--hidden','--frames',"$($taskCase.frames)",'--capture',"tests/artifacts/v12/$($taskCase.name).bmp")+$taskCase.args
 $taskProcess=Start-Process -FilePath $taskExecutable -WorkingDirectory $taskRoot -ArgumentList $taskArguments -PassThru -Wait -WindowStyle Hidden
 $taskLog=[string](Get-Content -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Raw)
 Copy-Item -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Destination (Join-Path $taskArtifacts "$($taskCase.name).log")
 if($taskProcess.ExitCode -ne 0 -or $taskLog -notmatch 'GL errors: 0' -or $taskLog -notmatch 'Loaded 6 recorded footstep samples' -or $taskLog -notmatch 'Music loaded: yes' -or $taskLog -notmatch 'Loaded Smiler model: 8728 triangles' -or !$taskLog.ToLowerInvariant().Contains($taskCase.expect.ToLowerInvariant())){throw "$($taskCase.name) failed: $taskLog"}
 if($taskCase.name -eq 'natural-spawn') {
  if($taskLog -notmatch 'Threat: WANDERING / distance ([0-9.]+) m'){throw 'No natural spawn distance logged'}
  $taskDistance=[double]::Parse($Matches[1],[Globalization.CultureInfo]::InvariantCulture)
  if($taskDistance -lt 16 -or $taskDistance -gt 22){throw "Spawn outside closer range: $taskDistance"}
 }
 Write-Host "$($taskCase.name) passed"
 [pscustomobject]@{case=$taskCase.name;arguments=$taskArguments;exitCode=$taskProcess.ExitCode;log=$taskLog}
}
$taskResults | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskArtifacts 'results.json') -Encoding utf8
Write-Host 'Inspect captures in tests/artifacts/v12. These are automated callback checks, not a manual playtest.'
