param([string]$Executable='Backrooms.exe')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $taskRoot
$taskExecutable=Join-Path $taskRoot $Executable
$taskEvidence='tests/artifacts/jumpscare'
New-Item -ItemType Directory -Force $taskEvidence | Out-Null
$taskCases=@(
    @{name='normal';frames=30;args=@();expected='Catches: 0'},
    @{name='flash';frames=4;args=@('--auto-jumpscare');expected='Catches: 1'},
    @{name='closeup';frames=38;args=@('--auto-catch');expected='Encounter: jumpscare'},
    @{name='late';frames=110;args=@('--auto-catch');expected='Encounter: jumpscare'},
    @{name='black';frames=129;args=@('--auto-catch');expected='Encounter: jumpscare'},
    @{name='gameover';frames=150;args=@('--auto-catch');expected='Encounter: game-over'},
    @{name='restart';frames=170;args=@('--auto-catch','--auto-restart');expected='Restarts: 1. Key: missing. Hiding: no. Encounter: exploring'},
    @{name='debug-repeat';frames=205;args=@('--auto-jumpscare','--auto-restart');expected='Catches: 2. Restarts: 1'},
    @{name='proximity';frames=38;args=@('--threat-x','3.75','--threat-z','2.4');expected='Catches: 1'},
    @{name='crouched';frames=75;args=@('--auto-jumpscare','--auto-crouch');expected='Encounter: jumpscare'}
)
foreach($taskCase in $taskCases) {
    $taskArgs=@('--hidden','--demo','--frames',$taskCase.frames,'--capture',"$taskEvidence/$($taskCase.name).bmp")+$taskCase.args
    $taskP=Start-Process -FilePath $taskExecutable -WorkingDirectory $taskRoot -ArgumentList $taskArgs -WindowStyle Hidden -Wait -PassThru
    $taskLog=Get-Content backrooms.log -Raw
    Copy-Item backrooms.log "$taskEvidence/$($taskCase.name).log"
    if($taskP.ExitCode -ne 0 -or !$taskLog.Contains('GL errors: 0') -or !$taskLog.Contains($taskCase.expected) -or !$taskLog.Contains('Loaded supplied jumpscare roar.')) {
        throw "Failed $($taskCase.name): $taskLog"
    }
    if($taskCase.name -eq 'black' -or $taskCase.name -eq 'flash') {
        $taskPixels=[IO.File]::ReadAllBytes((Join-Path $taskRoot "$taskEvidence/$($taskCase.name).bmp"))
        $taskOffset=[BitConverter]::ToInt32($taskPixels,10)
        $taskValue=if($taskCase.name -eq 'black'){0}else{255}
        for($taskIndex=$taskOffset;$taskIndex -lt $taskPixels.Length;$taskIndex++) {
            if($taskPixels[$taskIndex] -ne $taskValue){throw "Unexpected pixel in $($taskCase.name)"}
        }
    }
    Write-Output "PASS $($taskCase.name)"
}
