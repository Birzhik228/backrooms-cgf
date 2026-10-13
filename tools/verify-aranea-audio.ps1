param([string]$CaseFilter='*')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskEvidence=Join-Path $taskRoot 'tests\artifacts\aranea-audio'
New-Item -ItemType Directory -Path $taskEvidence -Force | Out-Null
$taskCases=@(
 @{name='walk';frames=440;args=@('--auto-crouch','--yaw','0','--threat-x','28','--threat-z','3.75','--threat-yaw','90');kind='walk'},
 @{name='run-sighting';frames=100;args=@('--yaw','0','--threat-x','12','--threat-z','3.75','--threat-yaw','-90');kind='run'},
 @{name='paused';frames=100;args=@('--threat-x','8','--threat-z','3.75','--threat-yaw','-90');kind='paused'},
 @{name='key-roar';frames=220;args=@('--auto-key');kind='roar'},
 @{name='capture-restart';frames=180;args=@('--auto-catch','--auto-restart');kind='restart'}
)
$taskResults=@()
foreach($taskCase in $taskCases){
 if($taskCase.name -notlike $CaseFilter){continue}
 $taskArguments=@('--hidden','--width','960','--height','640','--frames',"$($taskCase.frames)")+ $taskCase.args
 if($taskCase.kind -ne 'paused'){$taskArguments+='--demo'}
 $taskProcess=Start-Process -FilePath (Join-Path $taskRoot 'Backrooms.exe') -WorkingDirectory $taskRoot -ArgumentList $taskArguments -WindowStyle Hidden -PassThru
 if(-not $taskProcess.WaitForExit(60000)){throw "Timed out: $($taskCase.name)"}
 $taskLog=Get-Content -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Raw
 Copy-Item -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Destination (Join-Path $taskEvidence ($taskCase.name+'.log')) -Force
 if($taskProcess.ExitCode -ne 0 -or $taskLog -notmatch 'GL errors: 0' -or $taskLog -notmatch 'Loaded Aranea audio: 16' -or $taskLog -notmatch 'Music loaded: yes'){
  throw "Startup/render failed: $($taskCase.name): $taskLog"
 }
 if($taskLog -notmatch 'Creature walk/run footfalls: (\d+)/(\d+)'){throw 'Missing foot contact summary'}
 $taskWalk=[int]$Matches[1];$taskRun=[int]$Matches[2]
 if($taskLog -notmatch 'Creature visual notices/roars: (\d+)/(\d+)'){throw 'Missing vocal summary'}
 $taskNotices=[int]$Matches[1];$taskRoars=[int]$Matches[2]
 switch($taskCase.kind){
  'walk' {if($taskWalk -lt 2){throw 'Walking fixture did not emit repeated walk contacts'}}
  'run' {if($taskRun -lt 2 -or $taskNotices -ne 1){throw 'Chase must emit run contacts and exactly one visual notice'}}
  'paused' {if($taskWalk+$taskRun+$taskNotices+$taskRoars -ne 0 -or $taskLog -notmatch 'Paused: yes'){throw 'Paused fixture emitted creature cues'}}
  'roar' {if($taskRoars -lt 1 -or $taskLog -notmatch 'Key: collected'){throw 'Key pickup did not retain the creature roar'}}
  'restart' {if($taskLog -notmatch 'Restarts: 1'){throw 'Capture/restart regressed'}}
 }
 $taskResults+=[pscustomobject]@{case=$taskCase.name;walk=$taskWalk;run=$taskRun;notices=$taskNotices;roars=$taskRoars;args=$taskArguments;log=$taskLog}
 Write-Output "PASS $($taskCase.name): walk=$taskWalk run=$taskRun notices=$taskNotices roars=$taskRoars"
}
$taskResults | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding utf8
