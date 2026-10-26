param([string]$CaseFilter='*')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskEvidence=Join-Path $taskRoot 'tests\artifacts\level-menu'
New-Item -ItemType Directory -Path $taskEvidence -Force | Out-Null
$taskCases=@(
 @{name='pause-menu';frames=8;args=@('--demo','--auto-pause');expected=@('Level menu: closed','Level menu opens/travels: 0/0','Paused: yes')},
 @{name='open-main';frames=8;args=@('--level-menu-check');expected=@('Level menu: open','Level menu opens/travels: 1/0','Paused: yes')},
 @{name='open-play';frames=8;args=@('--demo','--level-menu-check');expected=@('Level menu: open','Level menu opens/travels: 1/0','Paused: yes')},
 @{name='close-play';frames=12;args=@('--demo','--level-menu-close');expected=@('Level menu: closed','Level menu opens/travels: 1/0','Paused: yes')},
 @{name='close-main';frames=12;args=@('--level-menu-close');expected=@('Level menu: closed','Level menu opens/travels: 1/0','Paused: yes')},
 @{name='escape-back-resume';frames=12;args=@('--demo','--level-menu-resume');expected=@('Level menu: closed','Level menu opens/travels: 1/0','Paused: no')},
 @{name='click-back';frames=12;args=@('--demo','--level-menu-back-click');expected=@('Level menu: closed','Level menu opens/travels: 1/0','Paused: yes')},
 @{name='ctrl-tap-play';frames=8;args=@('--demo','--control-tap-check');expected=@('Level menu: closed','Level menu opens/travels: 0/0','Paused: no')},
 @{name='ctrl-tap-menu';frames=8;args=@('--control-tap-check');expected=@('Level menu: closed','Level menu opens/travels: 0/0','Paused: yes')},
 @{name='hold-crouch';frames=25;args=@('--demo','--control-hold-check');expected=@('Crouched: yes','Level menu opens/travels: 0/0','Paused: no')},
 @{name='hold-release';frames=40;args=@('--demo','--control-hold-check');expected=@('Crouched: no','Level menu opens/travels: 0/0','Paused: no')},
 @{name='pause-with-ctrl';frames=10;args=@('--demo','--control-pause-check');expected=@('Level menu: closed','Level menu opens/travels: 0/0','Paused: yes')},
 @{name='click-fun';frames=14;args=@('--demo','--level-menu-select','2','--level-menu-click');expected=@('Level selected: Fun','Level: Fun','Level menu opens/travels: 1/1','Paused: no','Key: missing')},
 @{name='keyboard-parking';frames=14;args=@('--level-menu-select','3');expected=@('Level selected: Parking','Level: Parking','Level menu opens/travels: 1/1','Paused: no','Circuits: 0/3')},
 @{name='parking-to-zero';frames=30;args=@('--demo','--parking','--auto-breakers','--level-menu-select','1');expected=@('CIRCUIT A ONLINE','Level selected: 0','Level: 0','Circuits: 0/3','Key: missing','Level menu opens/travels: 1/1')},
 @{name='zero-restart-key';frames=30;args=@('--demo','--auto-key','--level-menu-select','1');expected=@('Level selected: 0','Level: 0','Key: missing','Level menu opens/travels: 1/1')},
 @{name='fun-to-zero';frames=14;args=@('--demo','--level-fun','--level-menu-select','1','--level-menu-click');expected=@('Level selected: 0','Level: 0','Presents opened: 0','Level menu opens/travels: 1/1')},
 @{name='guide-to-menu';frames=8;args=@('--demo','--guide-check','--level-menu-check');expected=@('Level menu: open','Guide: closed','Level menu opens/travels: 1/0','Paused: yes')},
 @{name='capture-ignores-ctrl';frames=12;args=@('--demo','--auto-catch','--control-tap-check');expected=@('Encounter: jumpscare','Level menu opens/travels: 0/0')},
 @{name='blackout-ignores-ctrl';frames=132;args=@('--demo','--auto-catch','--control-tap-check','--level-menu-at','125');expected=@('Level menu: closed','Level menu opens/travels: 0/0','Catches: 1')},
 @{name='game-over-select';frames=160;args=@('--demo','--auto-catch','--level-menu-select','3','--level-menu-at','148');expected=@('Level selected: Parking','Encounter: exploring','Level: Parking','Level menu opens/travels: 1/1')},
 @{name='odd-size';frames=8;args=@('--demo','--level-menu-check','--width','1919','--height','1027');expected=@('Level menu: open','Level menu opens/travels: 1/0')},
 @{name='settings-back-resume';frames=12;args=@('--menu-check');expected=@('Level menu: closed','Level menu opens/travels: 0/0','Paused: no','Volume: 0.80')}
)
$taskResults=@()
foreach($taskCase in $taskCases){
 if($taskCase.name -notlike $CaseFilter){continue}
 $taskArgs=@('--hidden','--seed','12071998','--width','960','--height','640','--frames',"$($taskCase.frames)",'--capture',('tests/artifacts/level-menu/'+$taskCase.name+'.bmp'))+$taskCase.args
 $taskProcess=Start-Process -FilePath (Join-Path $taskRoot 'Backrooms.exe') -WorkingDirectory $taskRoot -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
 if(-not $taskProcess.WaitForExit(60000)){throw "Timed out: $($taskCase.name)"}
 $taskLog=Get-Content -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Raw
 Copy-Item -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Destination (Join-Path $taskEvidence ($taskCase.name+'.log')) -Force
 if($taskProcess.ExitCode -ne 0 -or $taskLog -notmatch 'GL errors: 0'){throw "Failed: $($taskCase.name): $taskLog"}
 foreach($taskExpected in $taskCase.expected){if(-not $taskLog.Contains($taskExpected)){throw "$($taskCase.name): missing $taskExpected`n$taskLog"}}
 $taskResults+=[pscustomobject]@{name=$taskCase.name;args=$taskArgs;passed=$true}
 Write-Output "PASS $($taskCase.name)"
}
$taskResults|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding utf8
