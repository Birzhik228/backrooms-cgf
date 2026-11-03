param([string]$CaseFilter='*')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskEvidence=Join-Path $taskRoot 'tests\artifacts\exploration'
New-Item -ItemType Directory -Path $taskEvidence -Force | Out-Null
# The fixed creature occupies real walkable space behind an ordinary closed
# door. Preview advances only the presence clock, not the creature or door.
$taskPresence=@('--presence-preview','--x','-5.5','--z','11.25','--yaw','0','--pitch','-5','--threat-x','3.75','--threat-z','11.25')
$taskCases=@(
 @{name='landmark-first';frames=3;args=@('--observation-preview','0','--no-threat','--camera-view');expected=@('Observation preview:','Level 0 notes read: 0')},
 @{name='landmark-second';frames=3;args=@('--observation-preview','1','--no-threat','--camera-view');expected=@('Observation preview:','Level 0 notes read: 0')},
 @{name='read-note';frames=8;args=@('--auto-read-observation','--no-threat');expected=@('Level 0 note read:','Level 0 notes read: 1','Note visible: yes','Doors used: 0','Key: missing','Paused: no')},
 @{name='read-looking-down';frames=8;args=@('--auto-read-observation','--no-threat','--pitch','-82');expected=@('Level 0 notes read: 1','Note visible: yes')},
 @{name='read-looking-away';frames=8;args=@('--auto-read-observation','--no-threat','--pitch','35');expected=@('Level 0 notes read: 0','Note visible: no')},
 @{name='read-note-paused';frames=8;args=@('--auto-read-observation','--auto-pause','--no-threat');expected=@('Level 0 notes read: 0','Note visible: no','Paused: yes')},
 @{name='note-level-change';frames=22;args=@('--auto-read-observation','--level-menu-select','2','--level-menu-at','6','--no-threat');expected=@('Level 0 notes read: 1','Note visible: no','Level: Fun','Level menu opens/travels: 1/1')},
 @{name='fun-isolation';frames=8;args=@('--level-fun','--auto-read-observation','--no-threat');expected=@('Level: Fun','Level 0 notes read: 0','Level 0 door hints: 0')},
 @{name='parking-isolation';frames=8;args=@('--parking','--auto-read-observation','--no-threat');expected=@('Level: Parking','Level 0 notes read: 0','Level 0 door hints: 0')},
 @{name='door-hint';frames=6;args=$taskPresence;expected=@('Level 0 presence: door handle','Level 0 door hints: 1','Door hint active: yes','Doors used: 0')},
 @{name='door-hint-settled';frames=65;args=$taskPresence;expected=@('Level 0 door hints: 1','Door hint active: no','Doors used: 0')},
 @{name='door-hint-pause';frames=65;args=$taskPresence+@('--auto-pause');expected=@('Level 0 door hints: 1','Door hint active: yes','Paused: yes')},
 @{name='no-fake-presence';frames=6;args=$taskPresence+@('--no-threat');expected=@('Level 0 door hints: 0','Door hint active: no')}
)
$taskResults=@()
foreach($case in $taskCases){
 if($case.name -notlike $CaseFilter){continue}
 $taskArgs=@('--hidden','--demo','--seed','12071998','--width','960','--height','640','--frames',"$($case.frames)",'--capture',('tests/artifacts/exploration/'+$case.name+'.bmp'))+$case.args
 $taskProcess=Start-Process -FilePath (Join-Path $taskRoot 'Backrooms.exe') -WorkingDirectory $taskRoot -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
 if(-not $taskProcess.WaitForExit(60000)){throw ('Timed out: '+$case.name)}
 $taskLog=Get-Content -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Raw
 Copy-Item -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Destination (Join-Path $taskEvidence ($case.name+'.log')) -Force
 if($taskProcess.ExitCode -ne 0 -or -not $taskLog.Contains('GL errors: 0')){throw ($case.name+': '+$taskLog)}
 foreach($expected in $case.expected){if(-not $taskLog.Contains($expected)){throw ($case.name+': missing '+$expected+"`n"+$taskLog)}}
 $taskResults+=[pscustomobject]@{name=$case.name;args=$taskArgs;passed=$true}
 Write-Output ('PASS '+$case.name)
}
$taskResults | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding utf8
