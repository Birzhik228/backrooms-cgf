param([string]$Executable = 'Backrooms.exe')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $taskRoot
$taskExecutable = Join-Path $taskRoot $Executable
if (-not (Test-Path -LiteralPath $taskExecutable)) { throw "Missing executable: $taskExecutable" }
$taskEvidence = Join-Path $taskRoot 'tests\artifacts'
New-Item -ItemType Directory -Force -Path $taskEvidence | Out-Null
# Fixed-step movement assertions allow the intended acceleration/braking travel.
# Former district coordinates must now use the same classic room style.
$taskCases = @(
    @{ Name='classic-hall'; Args=@('--demo','--frames','120','--metrics'); Velocity=0.0; Fov=78.0 },
    @{ Name='former-flooded'; Args=@('--demo','--frames','60','--x','-69','--z','3','--yaw','90') },
    @{ Name='former-garage'; Args=@('--demo','--frames','120','--x','111','--z','3','--yaw','35','--auto-sprint'); Velocity=0.0; Fov=78.0 },
    @{ Name='former-offices'; Args=@('--demo','--frames','60','--x','3','--z','111','--yaw','35') },
    @{ Name='former-gallery'; Args=@('--demo','--frames','60','--x','-105','--z','-105','--yaw','35') },
    @{ Name='camera-view'; Args=@('--demo','--frames','60','--yaw','-35','--camera-view','--no-flashlight') },
    @{ Name='wall-wear-close'; Args=@('--demo','--frames','60','--x','1.4','--z','1.0','--yaw','180','--pitch','-8','--camera-view') },
    @{ Name='wall-wear-other-chunk'; Args=@('--demo','--frames','60','--x','37.4','--z','1.0','--yaw','180','--pitch','-8','--camera-view') },
    @{ Name='soft-focus-wide'; Args=@('--demo','--frames','60','--yaw','-35','--camera-view','--width','1600','--height','900') },
    @{ Name='heavy-walk'; Args=@('--demo','--frames','120','--auto-walk'); ZMin=-1.9; ZMax=-1.4; Velocity=2.5; Fov=78.0 },
    @{ Name='crouch-sprint-priority'; Args=@('--demo','--frames','120','--auto-walk','--auto-crouch','--auto-sprint'); Expected='Eye height: 1.08. Crouched: yes'; ZMin=0.8; ZMax=1.0; Velocity=1.1; Fov=78.0 },
    @{ Name='stand-after-crouch'; Args=@('--demo','--frames','120','--auto-walk','--auto-crouch','--auto-stand'); Expected='Eye height: 1.65. Crouched: no'; ZMin=-0.6; ZMax=-0.2; Velocity=2.5; Fov=78.0 },
    @{ Name='sprint'; Args=@('--demo','--frames','120','--auto-walk','--auto-sprint'); ZMin=-4.3; ZMax=-3.8; Velocity=4.0; Fov=82.0 },
    @{ Name='sprint-stop'; Args=@('--demo','--frames','120','--auto-walk','--auto-sprint','--auto-stop'); ZMin=-0.7; ZMax=-0.2; Velocity=0.0; Fov=78.0 },
    @{ Name='wall-stop'; Args=@('--demo','--frames','180','--x','0.94','--z','3','--auto-walk'); ZMin=0.3; ZMax=0.6; X=0.94; Velocity=0.0; Fov=78.0 },
    @{ Name='map-and-clue'; Args=@('--demo','--frames','60','--yaw','-125.4','--map','--metrics') },
    @{ Name='escape'; Args=@('--demo','--frames','60','--x','4.8','--z','-69.9','--auto-exit'); Expected='Exit used: yes. Escaped: yes'; Velocity=0.0 },
    @{ Name='escape-continue'; Args=@('--demo','--frames','60','--x','4.8','--z','-69.9','--auto-exit','--auto-continue'); Expected='Exit used: yes. Escaped: no'; Velocity=0.0 },
    @{ Name='minimum-window'; Args=@('--frames','60','--width','960','--height','640'); Velocity=0.0 },
    @{ Name='distant-coordinates'; Args=@('--demo','--frames','120','--x','600000003','--z','-600000003') },
    @{ Name='streaming-walk'; Args=@('--demo','--frames','2400','--auto-walk','--metrics'); ZMin=-96.9; ZMax=-96.3; Velocity=2.5; Fov=78.0 }
)
$taskResults = @()
$taskCulture = [System.Globalization.CultureInfo]::InvariantCulture
foreach ($taskCase in $taskCases) {
    $taskCapture = 'tests/artifacts/' + $taskCase.Name + '.bmp'
    $taskArguments = @('--hidden','--capture',$taskCapture) + $taskCase.Args
    $taskWatch = [System.Diagnostics.Stopwatch]::StartNew()
    $taskProcess = Start-Process -FilePath $taskExecutable -WorkingDirectory $taskRoot -ArgumentList $taskArguments -PassThru -Wait -WindowStyle Hidden
    $taskWatch.Stop()
    $taskLog = Get-Content -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Raw -Encoding UTF8
    $taskLog | Set-Content -LiteralPath (Join-Path $taskEvidence ($taskCase.Name+'.log')) -Encoding utf8
    if ($taskProcess.ExitCode -ne 0 -or $taskLog -notmatch 'GL errors: 0' -or -not (Test-Path -LiteralPath (Join-Path $taskRoot $taskCapture))) {
        throw "Failed case $($taskCase.Name): exit=$($taskProcess.ExitCode). $taskLog"
    }
    if (-not $taskLog.Contains('Room: YELLOW HALLS')) { throw "Unexpected room style in $($taskCase.Name): $taskLog" }
    if (-not $taskLog.Contains('Loaded 6 recorded footstep samples.')) { throw "Recorded footstep bank missing in $($taskCase.Name): $taskLog" }
    if ($taskLog -match 'Figures:|Caught:|Nextbot textures') { throw "Removed entity runtime found in $($taskCase.Name): $taskLog" }
    if ($taskCase.Expected -and -not $taskLog.Contains($taskCase.Expected)) { throw "Unexpected gameplay state in $($taskCase.Name): $taskLog" }
    foreach ($taskField in @('Velocity','Fov')) {
        if ($taskCase.ContainsKey($taskField)) {
            $taskPattern = if ($taskField -eq 'Fov') { 'FOV: ([0-9]+(?:\.[0-9]+)?)' } else { 'Velocity: ([0-9]+(?:\.[0-9]+)?)' }
            if ($taskLog -notmatch $taskPattern) { throw "Missing $taskField in $($taskCase.Name): $taskLog" }
            $taskValue = [double]::Parse($Matches[1], $taskCulture)
            if ([Math]::Abs($taskValue - $taskCase[$taskField]) -gt 0.05) { throw "Unexpected $taskField in $($taskCase.Name): $taskLog" }
        }
    }
    if ($taskCase.ContainsKey('ZMin')) {
        if ($taskLog -notmatch 'Position: (-?[0-9]+(?:\.[0-9]+)?), (-?[0-9]+(?:\.[0-9]+)?)') { throw "Missing position in $($taskCase.Name): $taskLog" }
        $taskX = [double]::Parse($Matches[1], $taskCulture)
        $taskZ = [double]::Parse($Matches[2], $taskCulture)
        $taskExpectedX = if ($taskCase.ContainsKey('X')) { $taskCase.X } else { 3.0 }
        if ([Math]::Abs($taskX-$taskExpectedX) -gt 0.03 -or $taskZ -lt $taskCase.ZMin -or $taskZ -gt $taskCase.ZMax) {
            throw "Movement regression in $($taskCase.Name): $taskLog"
        }
    }
    $taskResults += [PSCustomObject]@{case=$taskCase.Name;exitCode=$taskProcess.ExitCode;elapsedSeconds=[Math]::Round($taskWatch.Elapsed.TotalSeconds,3);arguments=($taskArguments -join ' ');summary=(($taskLog.Trim() -split "`n" | Where-Object { $_ -match '^(Completed |Streaming:)' }) -join ' ')}
    Write-Host "PASS $($taskCase.Name)"
}
$taskResults | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding utf8
Write-Host 'All 21 rendering checks completed. Inspect the captures; this is not a human-input usability test.'


