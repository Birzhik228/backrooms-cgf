param([string]$CaseFilter='*')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $taskRoot
New-Item -ItemType Directory -Force tests/artifacts/room-details | Out-Null
$taskCases=@(
    @{name='guide-open';args='--guide-check --auto-walk --frames 4';expected=@('Guide: open','Paused: yes','Position: 3.75, 3.75','Threat: DORMANT')},
    @{name='guide-explore';args='--guide-check --guide-close --frames 6';expected=@('Guide: open','Guide page: 2','Paused: yes')},
    @{name='guide-survive';args='--guide-check --guide-close --frames 8';expected=@('Guide: open','Guide page: 3','Paused: yes')},
    @{name='guide-back';args='--guide-check --guide-close --guide-back --frames 9';expected=@('Guide: open','Guide page: 3','Guide page: 2','Paused: yes')},
    @{name='guide-skip';args='--guide-check --guide-skip --frames 8';expected=@('Controls popup closed.','Guide: closed','Paused: no')},
    @{name='guide-small';args='--guide-check --guide-close --frames 8 --width 960 --height 640';expected=@('Guide: open','Guide page: 3','Paused: yes')},
    @{name='guide-close';args='--guide-check --guide-close --frames 12';expected=@('Controls popup closed.','Guide: closed','Paused: no')},
    @{name='guide-click';args='--guide-check --guide-click --frames 12';expected=@('Controls popup closed.','Guide: closed','Paused: no')},
    @{name='mouse-contact';args='--demo --mouse-fixture --frames 60 --pitch -50';expected=@('Mouse squeaks: 1','Threat: SEARCHING')},
    @{name='mouse-preview';args='--demo --mouse-preview --auto-crouch --frames 3 --no-threat';expected=@('Mouse squeaks: 0')},
    @{name='wood-door';args='--demo --x -3.75 --z 2 --yaw -90 --frames 4 --no-threat';expected=@('Encounter: exploring')},
    @{name='white-exit';args='--demo --auto-key --auto-exit --frames 25 --yaw -90 --pitch 10 --no-flashlight --no-threat';expected=@('Key: collected')},
    @{name='vent-outside';args='--demo --hide-fixture --frames 30 --no-threat';expected=@('Hiding: no')},
    @{name='vent-inside';args='--demo --hide-fixture --auto-hide --frames 60 --no-threat';expected=@('Hiding: yes')}
)
foreach($taskCase in $taskCases) {
    if($taskCase.name -notlike $CaseFilter) {continue}
    $taskProcess=Start-Process -FilePath (Join-Path $taskRoot 'Backrooms.exe') -WorkingDirectory $taskRoot -ArgumentList ('--hidden '+$taskCase.args+' --capture tests/artifacts/room-details/'+$taskCase.name+'.bmp') -WindowStyle Hidden -Wait -PassThru
    $taskLog=Get-Content backrooms.log -Raw
    Copy-Item backrooms.log ('tests/artifacts/room-details/'+$taskCase.name+'.log')
    if($taskProcess.ExitCode -ne 0 -or !$taskLog.Contains('GL errors: 0')) {throw "$($taskCase.name): $taskLog"}
    foreach($taskExpected in $taskCase.expected) {if(!$taskLog.Contains($taskExpected)) {throw "$($taskCase.name): missing $taskExpected`n$taskLog"}}
    Write-Output "PASS $($taskCase.name)"
}
