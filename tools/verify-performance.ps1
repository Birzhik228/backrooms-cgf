param(
    [ValidateRange(1,1000)][int]$Cycles=9,
    [ValidateRange(1,86400)][int]$SoakSeconds=600,
    [switch]$SkipSoak,
    [string]$Executable='',
    [ValidateRange(1,4096)][double]$MemoryGrowthLimitMiB=64
)
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
if(!$Executable){$Executable=Join-Path $taskRoot 'Backrooms.exe'}
elseif(![IO.Path]::IsPathRooted($Executable)){$Executable=Join-Path $taskRoot $Executable}
$taskExecutable=(Resolve-Path -LiteralPath $Executable).Path
$taskExeDirectory=Split-Path -Parent $taskExecutable
$taskAssetRoot=if(Test-Path -LiteralPath (Join-Path $taskExeDirectory 'vshader_backrooms.glsl')){$taskExeDirectory}else{$taskRoot}
$taskEvidence=Join-Path $taskRoot ('tests\artifacts\performance\'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $taskEvidence -Force | Out-Null

function Assert-Performance([bool]$Condition,[string]$Message){if(!$Condition){throw $Message}}
function Median-Value($Values){
    $sorted=@($Values | Sort-Object)
    if(!$sorted.Count){return 0.0}
    $middle=[int][Math]::Floor($sorted.Count/2)
    if($sorted.Count%2){return [double]$sorted[$middle]}
    return ([double]$sorted[$middle-1]+[double]$sorted[$middle])/2
}
function Quote-Argument([string]$Value){
    # Generated arguments are plain paths/numbers; embedded quotes are invalid.
    Assert-Performance (!$Value.Contains('"')) 'An argument contains an embedded quote.'
    if($Value -match '\s'){return '"'+$Value+'"'}
    return $Value
}
function Check-Memory($Samples,[string]$Label){
    $rows=@($Samples)
    $resident=@($rows | Where-Object {$_.resident_known} | ForEach-Object {[double]$_.resident_bytes})
    $private=@($rows | Where-Object {$_.private_known} | ForEach-Object {[double]$_.private_bytes})
    $selected=if($private.Count -eq $rows.Count -and $rows.Count){$private}else{$resident}
    $metric=if($private.Count -eq $rows.Count -and $rows.Count){'private_bytes'}else{'resident_bytes'}
    $checked=$selected.Count -ge 6
    $baseline=0.0;$late=0.0;$growth=0.0
    if($selected.Count){
        $baseline=Median-Value @($selected | Select-Object -First ([Math]::Min(3,$selected.Count)))
        $late=Median-Value @($selected | Select-Object -Last ([Math]::Min(3,$selected.Count)))
        $growth=$late-$baseline
    }
    if($checked){
        $lateMinimum=($selected | Select-Object -Last 3 | Measure-Object -Minimum).Minimum
        $limit=$MemoryGrowthLimitMiB*1048576
        # Require three consecutive late readings above the warmed baseline;
        # an isolated peak or a different level's working set is not a leak.
        Assert-Performance (!($growth -gt $limit -and $lateMinimum-$baseline -gt $limit)) (
            "$Label sustained memory growth exceeds $MemoryGrowthLimitMiB MiB: "+[Math]::Round($growth/1048576,2)+' MiB')
    }
    return [pscustomobject]@{
        group=$Label;samples=$rows.Count;metric=$metric;sustainedGrowthChecked=$checked
        baselineMedianMiB=[Math]::Round($baseline/1048576,2)
        lateMedianMiB=[Math]::Round($late/1048576,2)
        growthMiB=[Math]::Round($growth/1048576,2)
        lastResidentMiB=if($resident.Count){[Math]::Round($resident[-1]/1048576,2)}else{$null}
        sampledPeakResidentMiB=if($resident.Count){[Math]::Round(($resident | Measure-Object -Maximum).Maximum/1048576,2)}else{$null}
    }
}
function Check-Plateau($Samples,[string]$Label){
    $rows=@($Samples)
    if($rows.Count -lt 2){return [pscustomobject]@{group=$Label;samples=$rows.Count;checked=$false}}
    # A frame can have zero, one or two live fences. Exclude that transient kind.
    for($kind=0;$kind -lt 6;++$kind){
        $counts=@($rows | ForEach-Object {[long]$_.alive[$kind]})
        $span=$counts | Measure-Object -Minimum -Maximum
        Assert-Performance ($span.Minimum -eq $span.Maximum) "$Label GL resource kind $kind does not plateau after warmup."
    }
    return [pscustomobject]@{group=$Label;samples=$rows.Count;checked=$true;aliveWithoutSync=@($rows[-1].alive | Select-Object -First 6)}
}
function Invoke-PerformanceCase([string]$Name,[string[]]$CaseArguments,[int]$TimeoutSeconds){
    $existing=@(Get-Process -Name ([IO.Path]::GetFileNameWithoutExtension($taskExecutable)) -ErrorAction SilentlyContinue |
        Where-Object {$_.Path -eq $taskExecutable})
    Assert-Performance ($existing.Count -eq 0) 'Close the already running instance of this executable before verification.'
    $json=Join-Path $taskEvidence ($Name+'.json')
    $arguments=@('--hidden','--demo','--seed','42','--width','960','--height','640',
        '--performance-report',$json,'--slow-frame-ms','50')+$CaseArguments
    $encoded=@($arguments | ForEach-Object {Quote-Argument $_})
    $watch=[Diagnostics.Stopwatch]::StartNew()
    $process=Start-Process -FilePath $taskExecutable -WorkingDirectory $taskAssetRoot -ArgumentList $encoded -WindowStyle Hidden -PassThru
    $progressAt=30.0
    while(!$process.WaitForExit(1000)){
        if($watch.Elapsed.TotalSeconds -gt $TimeoutSeconds){
            $process.Kill();$process.WaitForExit()
            throw "$Name timed out after $TimeoutSeconds seconds (only its verification process was stopped)."
        }
        if($watch.Elapsed.TotalSeconds -ge $progressAt){
            Write-Host ('RUN '+$Name+' / elapsed '+[int]$watch.Elapsed.TotalSeconds+' s')
            $progressAt+=30
        }
    }
    $watch.Stop()
    $logPath=Join-Path $taskAssetRoot 'backrooms.log'
    $log=Get-Content -LiteralPath $logPath -Raw
    Copy-Item -LiteralPath $logPath -Destination (Join-Path $taskEvidence ($Name+'.log'))
    Assert-Performance ($process.ExitCode -eq 0) "$Name game process failed: $log"
    Assert-Performance ($log.Contains('GL errors: 0') -and $log.Contains('GL resources after Game: 0')) "$Name did not finish with clean OpenGL/resource state."
    Assert-Performance (Test-Path -LiteralPath $json) "$Name did not write a performance report."
    $report=Get-Content -LiteralPath $json -Raw | ConvertFrom-Json
    Assert-Performance ($report.schema -eq 1 -and $report.frames -gt 0) "$Name invalid performance report."
    Assert-Performance (@($report.final_alive).Count -eq 7 -and @($report.created).Count -eq 7 -and @($report.destroyed).Count -eq 7) "$Name invalid resource arrays."
    for($kind=0;$kind -lt 7;++$kind){
        Assert-Performance ($report.final_alive[$kind] -eq 0) "$Name retains resource kind $kind after Game destruction."
        Assert-Performance ($report.created[$kind] -eq $report.destroyed[$kind]) "$Name resource acquisition/release totals do not balance for kind $kind."
    }
    $samples=@($report.samples)
    Assert-Performance ($samples.Count -le 720 -and $samples.Count -eq $report.samples_retained) "$Name sample history exceeds its bound."
    Assert-Performance (@($report.worst_frames).Count -le 16) "$Name slow-frame history exceeds its bound."
    foreach($sample in $samples){
        Assert-Performance ($sample.chunks -ge 0 -and $sample.chunks -le 25 -and $sample.prepared -ge 0 -and $sample.prepared -le 24 -and $sample.gpu_chunks -ge 0 -and $sample.gpu_chunks -le 49) "$Name world/GPU chunk working set exceeds 25 + 24."
        Assert-Performance (@($sample.alive).Count -eq 7 -and $sample.alive[6] -le 2) "$Name has an invalid resource sample or unbounded GPU fences."
    }
    return [pscustomobject]@{name=$Name;report=$report;log=$log;json=$json;elapsedSeconds=$watch.Elapsed.TotalSeconds;arguments=$arguments}
}
function Summarize-Case($Case,$Memory,$Plateaus){
    $r=$Case.report
    return [pscustomobject]@{
        name=$Case.name;report=$Case.json;elapsedWallSeconds=[Math]::Round($Case.elapsedSeconds,2)
        monitorDurationSeconds=$r.duration_seconds;frames=$r.frames;frameMs=$r.frame_ms;workMs=$r.work_ms
        slowFrames=$r.slow_frames;peakResidentMiB=[Math]::Round($r.peak_resident_bytes/1048576,2)
        peakSampledPrivateMiB=[Math]::Round($r.peak_sampled_private_bytes/1048576,2)
        privateMemoryKind=$r.private_memory_kind;peakObservedGlObjects=$r.peak_observed_gl_objects
        ownershipBalanced=$true;retainedSamples=$r.samples_retained;totalSamples=$r.samples_total
        plateauChecks=@($Plateaus);memoryChecks=@($Memory)
    }
}

$results=@()
$cycle=Invoke-PerformanceCase 'cycles' @('--no-threat','--fps-limit','0','--stress-cycles',"$Cycles",'--stress-interval','120') ([Math]::Max(180,$Cycles*40))
$expectedTransitions=$Cycles*3
Assert-Performance ($cycle.log -match ('(?m)^Stress transitions: '+$expectedTransitions+'\s*$')) 'The requested number of real level transitions was not completed.'
$cycleSamples=@($cycle.report.samples)
if($cycle.report.samples_total -le 720){
    foreach($event in @('level-leaving','level-ready')){
        Assert-Performance (@($cycleSamples | Where-Object {$_.event -eq $event}).Count -eq $expectedTransitions) "Missing $event lifecycle samples."
    }
}
$memory=@();$plateaus=@()
foreach($level in 0,1,2){
    $settled=@($cycleSamples | Where-Object {$_.event -eq 'level-settled' -and $_.level -eq $level})
    Assert-Performance ($settled.Count -ge 1) "No settled sample for level $level."
    $warm=@($settled | Select-Object -Skip 1)
    $plateaus+=Check-Plateau $warm "cycles-level-$level"
    $memory+=Check-Memory $warm "cycles-level-$level"
}
$results+=Summarize-Case $cycle $memory $plateaus
Write-Host "PASS cycles / $expectedTransitions real level transitions / resources balanced"
if(!$SkipSoak){
    # Active Parking rendering with a camera tour avoids ending up in a death
    # menu. This is intentionally not a player-navigation or FPS benchmark.
    $soak=Invoke-PerformanceCase 'parking-soak' @('--parking','--tour','--no-threat','--fps-limit','60','--run-seconds',"$SoakSeconds") ($SoakSeconds+180)
    Assert-Performance ($soak.elapsedSeconds -ge $SoakSeconds-.25 -and $soak.report.duration_seconds -ge $SoakSeconds-.25) 'The soak did not run for the requested real wall-clock duration.'
    $warmup=[Math]::Min(30,$SoakSeconds*.25)
    $steady=@($soak.report.samples | Where-Object {$_.event -eq 'tick' -and $_.level -eq 2 -and $_.seconds -ge $warmup})
    Assert-Performance ($steady.Count -gt 0) 'The soak did not sample active Parking gameplay.'
    $memory=@(Check-Memory $steady 'parking-soak')
    $plateaus=@(Check-Plateau $steady 'parking-soak')
    $results+=Summarize-Case $soak $memory $plateaus
    Write-Host "PASS parking-soak / $SoakSeconds real seconds / resources balanced"
}
$summary=[pscustomobject]@{
    executable=$taskExecutable;cycles=$Cycles;requestedTransitions=$expectedTransitions
    requestedSoakSeconds=$SoakSeconds;soakExecuted=(!$SkipSoak);memoryGrowthLimitMiB=$MemoryGrowthLimitMiB
    notes=@('Process RAM is not GPU VRAM.','Timing includes the configured FPS cap; no hardware FPS threshold is asserted.',
        'Cycle gameplay advances at a fixed simulation step; run-seconds and timings use actual wall time.',
        'Parking soak is a fixed-position camera tour without threats, not navigation coverage.',
        'Memory trend checks require six warmed same-level samples; shorter runs explicitly report unchecked trends.')
    cases=$results
}
$summary | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $taskEvidence 'validation.json') -Encoding utf8
Write-Host ('Performance evidence: '+$taskEvidence)
