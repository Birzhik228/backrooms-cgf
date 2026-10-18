param([string]$CaseFilter='*')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $taskRoot
New-Item -ItemType Directory -Force tests/artifacts/fun | Out-Null
$taskCases=@(
 @{name='party-room';args='--level-fun --yaw -125 --pitch -8 --frames 30';expected=@('Level: Fun','Mouse active: no','Threat: DORMANT')},
 @{name='seed42';args='--level-fun --seed 42 --yaw -125 --pitch -8 --frames 30';expected=@('Level: Fun','Mouse active: no')},
 @{name='table-detail';args='--level-fun --x 1.65 --z 3.75 --yaw -90 --pitch -28 --frames 30';expected=@('Level: Fun')},
 @{name='transition';args='--auto-key --auto-exit --auto-walk --frames 120';expected=@('Transition: Level 0 -> Level Fun. Partygoers enabled; Aranea and mouse disabled.','Exit used: yes','Level: Fun','Mouse active: no','Threat: DORMANT','Encounter: exploring')},
 @{name='entity-guards';args='--level-fun --auto-jumpscare --threat-x 8 --frames 60';expected=@('Level: Fun','Mouse active: no','Threat: DORMANT','Catches: 0','Mouse squeaks: 0','Creature footfalls: 0','Creature visual notices/roars: 0/0')},
 @{name='streaming';args='--level-fun --x 3.75 --z -42 --auto-walk --auto-sprint --frames 180 --map';expected=@('Level: Fun','synchronous fallback chunks 0')},
 @{name='guide';args='--level-fun --guide-check --guide-close --frames 8';expected=@('Level: Fun','Guide: open','Paused: yes')},
 @{name='verity-drop';args='--level-fun --seed 3 --auto-key --no-threat --frames 3';expected=@('Verity dropped from present.','Verity: inactive')},
 @{name='verity-pickup';args='--seed 3 --auto-verity --auto-walk --auto-sprint --no-threat --frames 300';expected=@('Verity collected. Infinite stamina enabled for Level Fun.','Verity: active','Stamina: 100')},
 @{name='present-mouse';args='--level-fun --seed 2 --auto-key --frames 3';expected=@('Present mouse released / all Partygoers alerted once.','Mouse active: yes','Mouse squeaks: 1','Key: missing','Presents opened: 1')},
 @{name='red-exit';args='--level-fun --exit-preview --no-threat --frames 3';expected=@('Level: Fun','Key: missing','GL errors: 0')},
 @{name='contact';args='--level-fun --auto-catch --frames 10';expected=@('Partygoer contact: game over. Restart at Level Fun checkpoint.','Encounter: game-over','Level: Fun','Catches: 1')},
 @{name='checkpoint';args='--level-fun --auto-catch --auto-restart --frames 95';expected=@('Restarted at Level Fun checkpoint.','Encounter: exploring','Restarts: 1','Level: Fun','Key: missing','Presents opened: 0')},
 @{name='presents';args='--level-fun --auto-key --no-threat --frames 65';expected=@('key found.','Key: collected','Level: Fun','Exit used: no')},
 @{name='fun-exit';args='--level-fun --auto-key --auto-exit --auto-walk --no-threat --frames 200';expected=@('Transition: Level Fun -> Parking.','Level: Parking','Escaped: no','Circuits: 0/3','Key: missing')},
 @{name='pause';args='--level-fun --auto-pause --frames 10';expected=@('Level: Fun','Paused: yes')}
)
foreach($taskCase in $taskCases) {
 if($taskCase.name -notlike $CaseFilter){continue}
 $taskProc=Start-Process -FilePath (Join-Path $taskRoot 'Backrooms.exe') -WorkingDirectory $taskRoot -ArgumentList ('--hidden --demo '+$taskCase.args+' --capture tests/artifacts/fun/'+$taskCase.name+'.bmp') -WindowStyle Hidden -Wait -PassThru
 $taskLog=Get-Content backrooms.log -Raw
 Copy-Item backrooms.log ('tests/artifacts/fun/'+$taskCase.name+'.log')
 if($taskProc.ExitCode -ne 0 -or !$taskLog.Contains('GL errors: 0')) {throw "$($taskCase.name): $taskLog"}
 foreach($expected in $taskCase.expected) {if(!$taskLog.Contains($expected)){throw "$($taskCase.name): missing $expected`n$taskLog"}}
 Write-Output "PASS $($taskCase.name)"
}
