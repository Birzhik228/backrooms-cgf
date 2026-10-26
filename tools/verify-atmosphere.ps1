param([string]$CaseFilter='*')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskEvidence=Join-Path $taskRoot 'tests\artifacts\atmosphere'
New-Item -ItemType Directory -Path $taskEvidence -Force | Out-Null
$taskCases=@(
 @{name='room-and-wall-steps';frames=3600;args=@('--demo','--no-threat','--auto-crouch');mode='ambience'},
 @{name='natural-spawn';frames=1800;args=@('--demo','--auto-crouch');mode='spawn'},
 @{name='paused';frames=1800;args=@('--no-threat','--auto-crouch');mode='silent'},
 @{name='fun-isolation';frames=1200;args=@('--demo','--level-fun','--no-threat','--auto-crouch');mode='silent'},
 @{name='parking-isolation';frames=1200;args=@('--demo','--parking','--no-threat','--auto-crouch');mode='silent'},
 @{name='capture-restart';frames=180;args=@('--demo','--auto-catch','--auto-restart');mode='restart'}
)
$taskResults=@()
foreach($taskCase in $taskCases){
 if($taskCase.name -notlike $CaseFilter){continue}
 $taskArgs=@('--hidden','--seed','12071998','--width','960','--height','640','--frames',"$($taskCase.frames)")+$taskCase.args
 $taskProcess=Start-Process -FilePath (Join-Path $taskRoot 'Backrooms.exe') -WorkingDirectory $taskRoot -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
 # Poll in short waits so the caller can report progress while captures run.
 $taskDeadline=(Get-Date).AddMinutes(3)
 while(-not $taskProcess.WaitForExit(1000)){if((Get-Date) -gt $taskDeadline){throw "Timed out: $($taskCase.name)"}}
 $taskLog=Get-Content -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Raw
 Copy-Item -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Destination (Join-Path $taskEvidence ($taskCase.name+'.log')) -Force
 if($taskProcess.ExitCode -ne 0 -or $taskLog -notmatch 'GL errors: 0' -or $taskLog -notmatch 'Loaded Aranea audio: 16'){throw "Failed startup/render: $($taskCase.name): $taskLog"}
 if($taskLog -notmatch 'Level 0 ambience/steps/hints: (\d+)/(\d+)/(\d+)'){throw 'Missing atmosphere summary'}
 $taskAmbient=[int]$Matches[1];$taskSteps=[int]$Matches[2];$taskHints=[int]$Matches[3]
 switch($taskCase.mode){
  'ambience' {if($taskAmbient -lt 2 -or $taskSteps -lt 1 -or $taskHints -ne 0){throw 'Missing ambient/wall events or ghost Aranea hint'}}
  'silent' {if($taskAmbient+$taskSteps+$taskHints -ne 0){throw 'Atmosphere leaked into pause or another level'}}
  'spawn' {
   # A real mouse alarm can put the first ordinary spawn straight into Search.
   if($taskLog -notmatch 'Threat: (?:WANDERING|SEARCHING) / distance ([0-9.]+) m'){throw 'No natural spawn'}
   $taskDistance=[double]::Parse($Matches[1],[Globalization.CultureInfo]::InvariantCulture)
   if($taskDistance -lt 23.99 -or $taskDistance -gt 32.01){throw "Incorrect spawn distance $taskDistance"}
  }
  'restart' {if($taskLog -notmatch 'Restarts: 1'){throw 'Capture/restart failed'}}
 }
 $taskResults+=[pscustomobject]@{name=$taskCase.name;ambient=$taskAmbient;steps=$taskSteps;hints=$taskHints;args=$taskArgs}
 Write-Output "PASS $($taskCase.name): ambient=$taskAmbient steps=$taskSteps hints=$taskHints"
}
$taskResults | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding utf8
