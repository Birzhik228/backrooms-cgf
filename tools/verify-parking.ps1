param([string]$CaseFilter='*')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $taskRoot
New-Item -ItemType Directory -Force tests/artifacts/parking | Out-Null
$taskCases=@(
 @{name='arrival';args='--parking --yaw -55 --pitch -7 --frames 5';expected=@('Level: Parking','Circuits: 0/3','Mouse active: no','Threat: DORMANT')},
 @{name='eyes-off';args='--darkness-preview --no-flashlight --frames 3';expected=@('Darkness eyes: 1 / dispersed 0','Catches: 0')},
 @{name='eyes-on';args='--darkness-preview --frames 3';expected=@('Darkness eyes dispersed by flashlight.','Darkness eyes: 0 / dispersed 1','Catches: 0')},
 @{name='eyes-away';args='--darkness-preview --yaw 90 --frames 3';expected=@('Darkness eyes: 1 / dispersed 0','Catches: 0')},
 @{name='contact';args='--parking --auto-catch --frames 20';expected=@('Darkness sound: electrical shriek.','Darkness contact: jumpscare started.','Encounter: jumpscare','Catches: 1','Paused: yes')},
 @{name='game-over';args='--parking --auto-catch --frames 150';expected=@('Darkness contact: jumpscare started.','Encounter: game-over','Catches: 1')},
 @{name='checkpoint';args='--parking --auto-catch --auto-restart --frames 155';expected=@('Restarted at Parking checkpoint.','Encounter: exploring','Restarts: 1','Level: Parking','Circuits: 0/3')},
 @{name='natural-spawn';args='--parking --no-flashlight --auto-door --frames 400';expected=@('Darkness eyes: 3 / dispersed 0 / spawned 3','Catches: 0')},
 @{name='circuits';args='--parking --auto-breakers --frames 12';expected=@('CIRCUIT A ONLINE','CIRCUIT B ONLINE','CIRCUIT C ONLINE','SERVICE EXIT UNLOCKED','Circuits: 3/3','Key: missing')},
 @{name='completion';args='--parking --auto-breakers --auto-exit --auto-walk --frames 130';expected=@('Parking completed. All three circuits restored; service exit reached.','Escaped: yes','Level: Parking','Circuits: 3/3','Catches: 0')},
 @{name='locked';args='--parking --exit-preview --auto-exit --frames 10';expected=@('Doors used: 0','Escaped: no','Circuits: 0/3')},
 # The first queued E arrives on frame 2, after Escape pauses the game.
 @{name='pause';args='--parking --auto-breakers --auto-pause --frames 10';expected=@('Paused: yes','Circuits: 0/3')},
 @{name='guide';args='--parking --guide-check --guide-close --frames 8';expected=@('Level: Parking','Guide: open','Paused: yes')},
 @{name='streaming';args='--parking --x 3.75 --z -42 --auto-walk --auto-sprint --no-threat --frames 180 --map';expected=@('Level: Parking','synchronous fallback chunks 0')}
)
foreach($taskCase in $taskCases) {
 if($taskCase.name -notlike $CaseFilter){continue}
 $taskProc=Start-Process -FilePath (Join-Path $taskRoot 'Backrooms.exe') -WorkingDirectory $taskRoot -ArgumentList ('--hidden --demo '+$taskCase.args+' --capture tests/artifacts/parking/'+$taskCase.name+'.bmp') -WindowStyle Hidden -Wait -PassThru
 $taskLog=Get-Content backrooms.log -Raw
 Copy-Item backrooms.log ('tests/artifacts/parking/'+$taskCase.name+'.log')
 if($taskProc.ExitCode -ne 0 -or !$taskLog.Contains('GL errors: 0')) {throw "$($taskCase.name): $taskLog"}
 foreach($expected in $taskCase.expected) {if(!$taskLog.Contains($expected)){throw "$($taskCase.name): missing $expected`n$taskLog"}}
 Write-Output "PASS $($taskCase.name)"
}
