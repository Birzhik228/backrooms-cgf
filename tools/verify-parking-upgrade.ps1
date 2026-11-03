param([string]$CaseFilter='*')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskEvidence=Join-Path $taskRoot 'tests/artifacts/parking-upgrade'
New-Item -ItemType Directory -Path $taskEvidence -Force | Out-Null
$taskCases=@(
 @{name='power-hud';frames=3;args=@('--parking','--no-threat','--map');expected=@('Circuit states: A OFF / B OFF / C OFF');pattern='Circuit target: [ABC]'},
 @{name='partial-power';frames=4;args=@('--parking','--no-threat','--auto-breakers','--map');expected=@('Circuit states: A ON / B OFF / C OFF','Parking pipes/drips/vents/relays: 0/0/0/1','Parking power relay: A')},
 @{name='sector-signs';frames=3;args=@('--parking','--seed','42','--no-threat','--x','3.75','--z','3.75','--yaw','45','--pitch','3','--camera-view');expected=@('Level: Parking')},
 @{name='vent-grille';frames=3;args=@('--parking','--seed','42','--no-threat','--x','26.25','--z','3.75','--yaw','-20','--pitch','34','--camera-view');expected=@('Level: Parking')},
 @{name='pipe-leak';frames=3;args=@('--parking','--seed','42','--no-threat','--x','33.75','--z','3.75','--yaw','174','--pitch','-29','--camera-view');expected=@('Level: Parking')},
 @{name='floor-cable';frames=3;args=@('--parking','--seed','42','--no-threat','--x','26.25','--z','11.25','--yaw','-20','--pitch','-35','--camera-view');expected=@('Level: Parking')},
 @{name='cabinet-marking';frames=3;args=@('--parking','--seed','42','--no-threat','--x','-46.675','--z','-18.5','--yaw','-90','--pitch','-20','--camera-view');expected=@('Level: Parking')},
 @{name='ambience';frames=6;args=@('--parking','--no-threat','--parking-ambience-preview');expected=@('Parking ambience: pipe tick');pattern='Parking pipes/drips/vents/relays: [1-9][0-9]*/[0-9]+/0/0'},
 @{name='powered-ambience';frames=18;args=@('--parking','--no-threat','--auto-breakers','--parking-ambience-preview','--map');expected=@('Circuit states: A ON / B ON / C ON','Circuit target: none','Parking ambience: ventilation','Parking power relay: A','Parking power relay: B','Parking power relay: C');pattern='Parking pipes/drips/vents/relays: [0-9]+/[0-9]+/[1-9][0-9]*/3'},
 @{name='paused-ambience';frames=6;args=@('--parking','--no-threat','--parking-ambience-preview','--auto-pause');expected=@('Paused: yes','Parking pipes/drips/vents/relays: 0/0/0/0')},
 @{name='leave-parking';frames=32;args=@('--parking','--no-threat','--auto-breakers','--level-menu-select','2','--level-menu-at','16');expected=@('Level: Fun','Circuit states: inactive','Circuit target: none','Level menu opens/travels: 1/1','Parking pipes/drips/vents/relays: 0/0/0/3')},
 @{name='level0-isolation';frames=6;args=@('--no-threat','--parking-ambience-preview');expected=@('Level: 0','Circuit states: inactive','Parking pipes/drips/vents/relays: 0/0/0/0')},
 @{name='fun-isolation';frames=6;args=@('--level-fun','--no-threat','--parking-ambience-preview');expected=@('Level: Fun','Circuit states: inactive','Parking pipes/drips/vents/relays: 0/0/0/0')}
)
$taskResults=@()
foreach($case in $taskCases){
 if($case.name -notlike $CaseFilter){continue}
 $taskArgs=@('--hidden','--demo','--seed','12071998','--width','960','--height','640','--frames',"$($case.frames)",'--capture',('tests/artifacts/parking-upgrade/'+$case.name+'.bmp'))+$case.args
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
