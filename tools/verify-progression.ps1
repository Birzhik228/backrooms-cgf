param([string]$CaseFilter='*')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $taskRoot
New-Item -ItemType Directory -Force tests/artifacts/progression | Out-Null
$taskCases=@(
    @{name='key';args='--key-preview --pitch -22 --frames 30 --map --no-threat';expected='Key: missing'},
    @{name='key-collected';args='--auto-key --frames 60 --map';expected='Key: collected'},
    @{name='locked-exit';args='--exit-preview --yaw -90 --auto-exit --frames 60 --no-threat';expected='Exit used: no. Escaped: no'},
    @{name='escape';args='--auto-key --auto-exit --auto-walk --frames 120 --no-threat';expected='Level: Fun'},
    @{name='recess';args='--hide-fixture --frames 30 --map --no-threat';expected='Hiding: no'},
    @{name='hidden';args='--hide-fixture --auto-hide --frames 60 --no-threat';expected='Hiding: yes'},
    @{name='leave-cover';args='--hide-fixture --auto-hide --frames 110 --no-threat';expected='Hiding: no'},
    @{name='recess-seed42';args='--seed 42 --hide-fixture --frames 30 --no-threat';expected='Hiding: no'},
    @{name='hidden-seed42';args='--seed 42 --hide-fixture --auto-hide --frames 60 --no-threat';expected='Hiding: yes'},
    @{name='reset-key';args='--auto-key --auto-jumpscare --auto-restart --frames 165';expected='Key: missing'}
)
foreach($taskCase in $taskCases) {
    if($taskCase.name -notlike $CaseFilter) { continue }
    $taskArgs='--hidden --demo '+$taskCase.args+' --capture tests/artifacts/progression/'+$taskCase.name+'.bmp'
    $taskProcess=Start-Process -FilePath (Join-Path $taskRoot 'Backrooms.exe') -WorkingDirectory $taskRoot -ArgumentList $taskArgs -WindowStyle Hidden -Wait -PassThru
    $taskLog=Get-Content backrooms.log -Raw
    Copy-Item backrooms.log ('tests/artifacts/progression/'+$taskCase.name+'.log')
    if($taskProcess.ExitCode -ne 0 -or !$taskLog.Contains('GL errors: 0') -or !$taskLog.Contains($taskCase.expected) -or !$taskLog.Contains('Loaded supplied heavy footsteps.')) {throw "$($taskCase.name): $taskLog"}
    if($taskCase.name -eq 'leave-cover' -and (!$taskLog.Contains('Entered hiding place.') -or !$taskLog.Contains('Left hiding place.'))) {throw 'Cover lifecycle did not execute'}
    if($taskCase.name -eq 'reset-key' -and (!$taskLog.Contains('Key collected.') -or !$taskLog.Contains('Restarts: 1'))) {throw 'Key reset lifecycle did not execute'}
    Write-Output "PASS $($taskCase.name)"
}
