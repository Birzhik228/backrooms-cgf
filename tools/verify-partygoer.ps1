param([string]$CaseFilter='*')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $taskRoot
New-Item -ItemType Directory -Force tests/artifacts/partygoer | Out-Null
$taskCases=@(
 @{name='portrait';args='--partygoer-preview --frames 30 --yaw -90 --pitch 5';expected='Partygoers: 4'},
 @{name='gaze';args='--partygoer-check --frames 80';expected='Partygoers: 4'},
 @{name='paused';args='--partygoer-preview --auto-pause --frames 60';expected='Paused: yes'},
 @{name='disabled';args='--level-fun --no-threat --frames 30';expected='Partygoers: 0'},
 @{name='level-zero';args='--frames 30';expected='Partygoers: 0'}
)
foreach($taskCase in $taskCases) {
 if($taskCase.name -notlike $CaseFilter){continue}
 $taskProc=Start-Process -FilePath (Join-Path $taskRoot 'Backrooms.exe') -WorkingDirectory $taskRoot -ArgumentList ('--hidden --demo --width 1280 --height 800 '+$taskCase.args+' --capture tests/artifacts/partygoer/'+$taskCase.name+'.bmp') -WindowStyle Hidden -Wait -PassThru
 $taskLog=Get-Content backrooms.log -Raw
 Copy-Item backrooms.log ('tests/artifacts/partygoer/'+$taskCase.name+'.log')
 if($taskProc.ExitCode -ne 0){throw "$($taskCase.name): exit $($taskProc.ExitCode)"}
 foreach($expected in @('GL errors: 0','Catches: 0','Encounter: exploring',$taskCase.expected)) {
  if(!$taskLog.Contains($expected)){throw "$($taskCase.name): missing $expected"}
 }
 if($taskCase.name -eq 'gaze' -and $taskLog -notmatch 'frozen frames [1-9][0-9]* / travelled [1-9]') {throw 'Expected both watching and moving in gaze fixture'}
 Write-Output "PASS $($taskCase.name)"
}
