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
 @{name='entity-guards';args='--level-fun --auto-catch --auto-jumpscare --threat-x 8 --frames 600';expected=@('Level: Fun','Mouse active: no','Threat: DORMANT','Catches: 0','Mouse squeaks: 0','Creature footfalls: 0','Creature visual notices/roars: 0/0')},
 @{name='streaming';args='--level-fun --x 3.75 --z -42 --auto-walk --auto-sprint --frames 180 --map';expected=@('Level: Fun','synchronous fallback chunks 0')},
 @{name='guide';args='--level-fun --guide-check --guide-close --frames 8';expected=@('Level: Fun','Guide: open','Paused: yes')},
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
