param([string]$CaseFilter='*')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskEvidence=Join-Path $taskRoot 'tests/artifacts/fun-upgrade'
New-Item -ItemType Directory -Path $taskEvidence -Force | Out-Null
$taskCases=@(
 @{name='clues-map';frames=3;args=@('--level-fun','--no-threat','--map');expected=@('Present clue: visible','Presents opened: 0')},
 @{name='clues-disabled';frames=3;args=@('--level-fun','--no-threat','--map','--no-clues');expected=@('Present clue: off')},
 @{name='searched';frames=65;args=@('--level-fun','--auto-key','--no-threat','--map');expected=@('Key: collected','Present clue: off')},
 @{name='toy-box';frames=3;args=@('--level-fun','--seed','42','--no-threat','--x','3.2','--z','2.85','--yaw','-131','--pitch','-18','--camera-view');expected=@('Level: Fun')},
 @{name='discarded-cups';frames=3;args=@('--level-fun','--no-threat','--x','26.25','--z','11.25','--yaw','44','--pitch','-29','--camera-view');expected=@('Level: Fun')},
 @{name='deflated-balloon';frames=3;args=@('--level-fun','--seed','0','--no-threat','--x','11.25','--z','33.75','--yaw','44','--pitch','-29','--camera-view');expected=@('Level: Fun')},
 @{name='ambience';frames=6;args=@('--level-fun','--no-threat','--fun-ambience-preview');expected=@('Level Fun ambience: balloon rub','Level Fun ambience: toy melody','Fun steps/balloons/melodies: 0/1/1')},
 @{name='ambience-paused';frames=6;args=@('--level-fun','--no-threat','--fun-ambience-preview','--auto-pause');expected=@('Fun steps/balloons/melodies: 0/0/0','Paused: yes','Present clue: off')},
 @{name='moving-partygoer';frames=85;args=@('--partygoer-check');expected=@('Frozen Partygoer steps: 0','Catches: 0');pattern='Fun steps/balloons/melodies: [1-9][0-9]*/0/0'},
 @{name='level-travel';frames=24;args=@('--level-fun','--fun-ambience-preview','--no-threat','--level-menu-select','3','--level-menu-at','8');expected=@('Level: Parking','Present clue: off','Level menu opens/travels: 1/1')},
 @{name='level0-isolation';frames=6;args=@('--no-threat','--fun-ambience-preview');expected=@('Level: 0','Fun steps/balloons/melodies: 0/0/0','Present clue: off')},
 @{name='parking-isolation';frames=6;args=@('--parking','--no-threat','--fun-ambience-preview');expected=@('Level: Parking','Fun steps/balloons/melodies: 0/0/0','Present clue: off')}
)
$taskResults=@()
foreach($case in $taskCases){
 if($case.name -notlike $CaseFilter){continue}
 $taskArgs=@('--hidden','--demo','--seed','12071998','--width','960','--height','640','--frames',"$($case.frames)",'--capture',('tests/artifacts/fun-upgrade/'+$case.name+'.bmp'))+$case.args
 $taskProcess=Start-Process -FilePath (Join-Path $taskRoot 'Backrooms.exe') -WorkingDirectory $taskRoot -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
 if(-not $taskProcess.WaitForExit(60000)){throw ('Timed out: '+$case.name)}
 $taskLog=Get-Content -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Raw
 Copy-Item -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Destination (Join-Path $taskEvidence ($case.name+'.log')) -Force
 if($taskProcess.ExitCode -ne 0 -or -not $taskLog.Contains('GL errors: 0')){throw ($case.name+': '+$taskLog)}
 foreach($expected in $case.expected){if(-not $taskLog.Contains($expected)){throw ($case.name+': missing '+$expected+"`n"+$taskLog)}}
 if($case.pattern -and $taskLog -notmatch $case.pattern){throw ($case.name+': '+$taskLog)}
 $taskResults+=[pscustomobject]@{name=$case.name;passed=$true;args=$taskArgs}
 Write-Output ('PASS '+$case.name)
}
$taskResults | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding utf8
