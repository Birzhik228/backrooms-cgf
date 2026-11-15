param([switch]$SkipSmoke)
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$taskExe=Join-Path $taskRoot 'Backrooms.exe'
function Assert-Submission([bool]$Condition,[string]$Message){if(!$Condition){throw $Message}}
function Resolve-Contained([string]$Base,[string]$Relative) {
    Assert-Submission (![IO.Path]::IsPathRooted($Relative)) "Absolute manifest path is invalid: $Relative"
    $path=[IO.Path]::GetFullPath((Join-Path $Base $Relative))
    Assert-Submission ($path.StartsWith($Base+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) "Manifest path escapes package: $Relative"
    return $path
}
function Test-ListedFile([string]$Base,$Entry) {
    $path=Resolve-Contained $Base $Entry.path
    Assert-Submission (Test-Path -LiteralPath $path -PathType Leaf) "Missing file: $($Entry.path)"
    $file=Get-Item -LiteralPath $path
    Assert-Submission (!($file.Attributes -band [IO.FileAttributes]::ReparsePoint)) "Linked file is not accepted: $($Entry.path)"
    $bytes=if($null -ne $Entry.bytes){$Entry.bytes}else{$Entry.size}
    Assert-Submission ($null -ne $bytes -and $file.Length -eq [long]$bytes) "Size mismatch: $($Entry.path)"
    Assert-Submission ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -eq $Entry.sha256) "SHA-256 mismatch: $($Entry.path)"
}
$taskManifestPath=Join-Path $taskRoot 'package-manifest.json'
Assert-Submission (Test-Path -LiteralPath $taskManifestPath -PathType Leaf) 'package-manifest.json is absent. Run this check from an extracted submission ZIP.'
$taskManifest=Get-Content -LiteralPath $taskManifestPath -Raw | ConvertFrom-Json
Assert-Submission ($taskManifest.schema -eq 1 -and @($taskManifest.files).Count -gt 0) 'Unsupported or empty package manifest.'
$taskSeen=@{}
foreach($taskEntry in $taskManifest.files) {
    Assert-Submission (!$taskSeen.ContainsKey($taskEntry.path)) "Duplicate manifest entry: $($taskEntry.path)"
    $taskSeen[$taskEntry.path]=$true
    Test-ListedFile $taskRoot $taskEntry
}
Write-Output ('PASS package hashes ('+@($taskManifest.files).Count+' files)')
$taskRequired=@('Run.cmd','Presentation.cmd','README_FIRST.txt','THIRD_PARTY.md','build.ps1','CMakeLists.txt',
    'vshader_backrooms.glsl','fshader_backrooms.glsl','vshader_lens.glsl','fshader_lens.glsl',
    'vshader_shadow.glsl','fshader_shadow.glsl','fshader_ao.glsl','balance.cfg','jumpscare.cfg',
    'assets/models/aranea/aranea.brm','assets/models/aranea/creature-color.rgb','assets/models/aranea/creature-normal.rgb',
    'assets/audio/music/threshold-ambient.wav','assets/asset-license-manifest.json',
    'docs/licenses/GLFW.txt','docs/licenses/GLAD.txt','docs/licenses/GCC-RUNTIME-EXCEPTION.txt')
if(!$taskManifest.sourceOnly -or !$SkipSmoke){$taskRequired+='Backrooms.exe'}
foreach($taskRelative in $taskRequired) {
    Assert-Submission (Test-Path -LiteralPath (Join-Path $taskRoot $taskRelative) -PathType Leaf) "Required runtime/source file missing: $taskRelative"
}
$taskAssets=Join-Path $taskRoot 'assets'
$taskLicense=Get-Content -LiteralPath (Join-Path $taskAssets 'asset-license-manifest.json') -Raw | ConvertFrom-Json
Assert-Submission (@($taskLicense.files).Count -gt 0) 'Asset license inventory is empty.'
$taskAssetSeen=@{}
foreach($taskEntry in $taskLicense.files) {
    Assert-Submission (!$taskAssetSeen.ContainsKey($taskEntry.path)) "Duplicate asset inventory entry: $($taskEntry.path)"
    $taskAssetSeen[$taskEntry.path]=$true
    foreach($taskField in @('license','source','creator')) {
        Assert-Submission (![string]::IsNullOrWhiteSpace([string]$taskEntry.$taskField)) "Asset lacks $taskField provenance: $($taskEntry.path)"
    }
    Test-ListedFile $taskAssets $taskEntry
}
foreach($taskFile in (Get-ChildItem -LiteralPath $taskAssets -File -Recurse)) {
    $taskRelative=$taskFile.FullName.Substring($taskAssets.Length+1).Replace('\','/')
    if($taskRelative -ne 'asset-license-manifest.json') {
        Assert-Submission ($taskAssetSeen.ContainsKey($taskRelative)) "Uninventoried asset: $taskRelative"
    }
}
Write-Output ('PASS asset hashes and provenance ('+$taskAssetSeen.Count+' files)')
if($SkipSmoke){Write-Output 'PASS manifest and required-file verification; smoke cases skipped.';return}
Assert-Submission (@(Get-Process -Name Backrooms -ErrorAction SilentlyContinue | Where-Object {$_.Path -eq $taskExe}).Count -eq 0) 'Close this package game before verification.'
$taskRelativeEvidence='tests/artifacts/submission/'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff')
$taskEvidence=Join-Path $taskRoot $taskRelativeEvidence
New-Item -ItemType Directory -Path $taskEvidence | Out-Null
$taskFixtureRelative=$taskRelativeEvidence+'/presentation.cfg'
@('ambientOcclusion 0','brightness 0.7','blur 1.2','fov 92','headBob 0.8','vsync 0','maxFps 0','musicEnabled 0') |
    Set-Content -LiteralPath (Join-Path $taskRoot $taskFixtureRelative) -Encoding ascii
$taskNormalRelative=$taskRelativeEvidence+'/gameplay.cfg'
@('ambientOcclusion 0.65','brightness 1','blur 0','fov 78','headBob 1','vsync 0','maxFps 0','musicEnabled 1') |
    Set-Content -LiteralPath (Join-Path $taskRoot $taskNormalRelative) -Encoding ascii
$taskCases=@(
    @{name='menu';frames=8;args=@();expected=@('Paused: yes')},
    @{name='presentation-ao-on';frames=12;presentation=$true;args=@('--presentation','--presentation-check','ao-on');expected=@('Presentation verified: ao-on','AO amount 0.850000')},
    @{name='presentation-ao-off';frames=12;presentation=$true;args=@('--presentation','--presentation-check','ao-off');expected=@('Presentation verified: ao-off','AO amount 0.000000')},
    @{name='presentation-inspection';frames=12;presentation=$true;args=@('--presentation','--presentation-check','camera');expected=@('Presentation verified: camera')},
    @{name='walking';frames=180;movement=$true;args=@('--demo','--auto-walk','--auto-sprint','--no-threat');expected=@('Paused: no','Encounter: exploring')},
    @{name='creature-capture';frames=12;args=@('--demo','--auto-jumpscare');expected=@('Catches: 1','Encounter: jumpscare')},
    @{name='creature-restart';frames=165;args=@('--demo','--auto-jumpscare','--auto-restart');expected=@('Restarts: 1','Encounter: exploring','Key: missing')},
    @{name='level0-to-fun';frames=120;args=@('--demo','--auto-key','--auto-exit','--auto-walk','--no-threat');expected=@('Level: Fun','Exit used: yes')},
    @{name='fun-to-parking';frames=200;args=@('--demo','--level-fun','--auto-key','--auto-exit','--auto-walk','--no-threat');expected=@('Transition: Level Fun -> Parking.','Level: Parking')},
    @{name='parking-to-ending';frames=130;args=@('--demo','--parking','--auto-breakers','--auto-exit','--auto-walk','--no-threat');expected=@('Parking completed.','Ending started: parking exit','Circuits: 3/3')},
    @{name='city-ending';frames=8;args=@('--demo','--ending-time','32');expected=@('Ending phase: City life','Ending city: 12 walking pedestrians / 6 moving cars.')},
    @{name='verity-pickup';frames=300;args=@('--demo','--seed','3','--auto-verity','--auto-walk','--auto-sprint','--no-threat');expected=@('Verity collected.','Verity: active')}
)
$taskResults=@()
foreach($taskCase in $taskCases) {
    $taskSettings=if($taskCase.presentation){$taskFixtureRelative}else{$taskNormalRelative}
    $taskArgs=@('--hidden','--seed','12071998','--width','1280','--height','720','--fps-limit','0',
        '--frames',"$($taskCase.frames)",'--settings-file',$taskSettings,
        '--capture',($taskRelativeEvidence+'/'+$taskCase.name+'.bmp'),
        '--profile',($taskRelativeEvidence+'/'+$taskCase.name+'.csv'))+$taskCase.args
    $taskClock=[Diagnostics.Stopwatch]::StartNew()
    Write-Output ('RUN '+$taskCase.name)
    $taskProcess=Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -ArgumentList $taskArgs -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $taskEvidence ($taskCase.name+'-stdout.txt')) `
        -RedirectStandardError (Join-Path $taskEvidence ($taskCase.name+'-stderr.txt'))
    # Windows PowerShell 5.1 must retain the process handle before it exits
    # to report ExitCode reliably when standard streams are redirected.
    $taskProcessHandle=$taskProcess.Handle
    while(!$taskProcess.WaitForExit(1000)) {
        if($taskClock.Elapsed.TotalSeconds -gt 120){$taskProcess.Kill();$taskProcess.WaitForExit();throw "Smoke case timed out: $($taskCase.name)"}
    }
    $taskProcess.WaitForExit()
    $taskProcess.Refresh()
    $taskClock.Stop()
    $taskLog=Get-Content -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Raw
    Copy-Item -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Destination (Join-Path $taskEvidence ($taskCase.name+'.log'))
    Assert-Submission ($taskProcess.ExitCode -eq 0) "$($taskCase.name): game exited with code $($taskProcess.ExitCode). See $taskEvidence"
    Assert-Submission ((Get-Item -LiteralPath (Join-Path $taskEvidence ($taskCase.name+'-stderr.txt'))).Length -eq 0) "$($taskCase.name): stderr is not empty."
    foreach($taskExpected in (@('GL errors: 0','GL resources after Game: 0','Background music loaded.')+$taskCase.expected)) {
        Assert-Submission ($taskLog.Contains($taskExpected)) "$($taskCase.name): missing $taskExpected. See $taskEvidence"
    }
    $taskBitmap=[IO.File]::ReadAllBytes((Join-Path $taskEvidence ($taskCase.name+'.bmp')))
    Assert-Submission ($taskBitmap.Length -ge 54) "$($taskCase.name): screenshot missing."
    Assert-Submission ([BitConverter]::ToInt32($taskBitmap,18) -eq 1280 -and [Math]::Abs([BitConverter]::ToInt32($taskBitmap,22)) -eq 720) "$($taskCase.name): framebuffer is not 1280x720."
    if($taskCase.movement) {
        $taskRows=@(Import-Csv -LiteralPath (Join-Path $taskEvidence ($taskCase.name+'.csv')))
        $taskX=[double]::Parse($taskRows[-1].x,[Globalization.CultureInfo]::InvariantCulture)
        $taskZ=[double]::Parse($taskRows[-1].z,[Globalization.CultureInfo]::InvariantCulture)
        Assert-Submission ([Math]::Sqrt([Math]::Pow($taskX-3.75,2)+[Math]::Pow($taskZ-3.75,2)) -gt 1) 'Walking fixture did not move at least one metre.'
    }
    $taskResults+=[pscustomobject][ordered]@{name=$taskCase.name;passed=$true;exitCode=$taskProcess.ExitCode;seconds=[Math]::Round($taskClock.Elapsed.TotalSeconds,3);width=1280;height=720;glErrors=0;retainedGlObjects=0;musicLoaded=$true;args=$taskArgs}
    $taskResults | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $taskEvidence 'cases.json') -Encoding utf8
    Write-Output ('PASS '+$taskCase.name)
}
$taskResult=[ordered]@{schema=1;completedUtc=(Get-Date).ToUniversalTime().ToString('o');packageManifestSha256=(Get-FileHash -LiteralPath $taskManifestPath -Algorithm SHA256).Hash.ToLowerInvariant();executableSha256=(Get-FileHash -LiteralPath $taskExe -Algorithm SHA256).Hash.ToLowerInvariant();verifiedPackageFiles=@($taskManifest.files).Count;verifiedAssetFiles=$taskAssetSeen.Count;cases=$taskResults;note='Hidden functional captures verify the extracted package, not visible gameplay FPS or audio listening.'}
$taskResult | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding utf8
Write-Output ('PASS all '+$taskResults.Count+' submission smoke cases')
Write-Output ('EVIDENCE '+$taskEvidence)
