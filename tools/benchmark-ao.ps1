param(
    [ValidateRange(1,10)][int]$Repetitions=3,
    [ValidateRange(1,60)][int]$WarmupSeconds=5,
    [ValidateRange(5,300)][int]$MeasureSeconds=30
)
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskExe=Join-Path $taskRoot 'Backrooms.exe'
$taskRelative='tests/artifacts/ao-performance/'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff')
$taskEvidence=Join-Path $taskRoot $taskRelative
New-Item -ItemType Directory -Path $taskEvidence -Force | Out-Null
function Assert-Benchmark([bool]$Condition,[string]$Message) { if(!$Condition){throw $Message} }
function Median($Values) {
    $sorted=@($Values | Sort-Object)
    $mid=[int][Math]::Floor($sorted.Count/2)
    if($sorted.Count%2){return [double]$sorted[$mid]}
    return ([double]$sorted[$mid-1]+[double]$sorted[$mid])/2
}
function Number([string]$Value) { return [double]::Parse($Value,[Globalization.CultureInfo]::InvariantCulture) }
function Settings-Hash {
    $path=Join-Path $taskRoot 'settings.cfg'
    if(Test-Path -LiteralPath $path){return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash}
    return 'absent'
}
Assert-Benchmark (Test-Path -LiteralPath $taskExe) 'Build Backrooms.exe first.'
Assert-Benchmark (@(Get-Process -Name Backrooms -ErrorAction SilentlyContinue).Count -eq 0) 'Close the running game before benchmarking.'
$taskSettingsBefore=Settings-Hash
$taskHardware=[ordered]@{
    cpu=Get-CimInstance Win32_Processor | Select-Object Name,NumberOfCores,NumberOfLogicalProcessors
    computer=Get-CimInstance Win32_ComputerSystem | Select-Object Manufacturer,Model,TotalPhysicalMemory
    adapters=@(Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion,DriverDate)
    os=Get-CimInstance Win32_OperatingSystem | Select-Object Caption,Version,BuildNumber
    batteryBefore=Get-CimInstance Win32_Battery | Select-Object BatteryStatus,EstimatedChargeRemaining
    powerScheme=(& powercfg /getactivescheme | Out-String).Trim()
}
$taskMetadata=[ordered]@{
    startedAt=(Get-Date).ToString('o');executableSha256=(Get-FileHash -LiteralPath $taskExe -Algorithm SHA256).Hash
    gitCommit=(& git -C $taskRoot rev-parse HEAD | Out-String).Trim()
    sourceStatus=@(& git -C $taskRoot status --short)
    hardware=$taskHardware;width=1280;height=720;seed=12071998
    scene='Frozen Level 0 presentation entrance';camera=@{x=3.75;z=3.75;yaw=-90;pitch=-5;fov=78}
    presentation=@{aoOn=0.85;aoOff=0;exposure=1.1;blur=0}
    hidden=$true;vsync=$false;fpsCap=0;boundedGpuQueue=$true;audioOutput=$false
    warmupSeconds=$WarmupSeconds;measureSeconds=$MeasureSeconds;repetitions=$Repetitions
    method='Discard complete frames until cumulative frame_ms reaches warmup; then include complete frames until measured frame_ms reaches duration. FPS = 1000 / mean(frame_ms). Percentiles use nearest rank on CSV samples.'
    limitation='Hidden frozen-scene rendering, not full gameplay or lab-PC certification. frame_ms includes CPU work, GPU queue waits and swap; it excludes small post-frame telemetry bookkeeping and is not isolated GPU time.'
}
$taskMetadata | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $taskEvidence 'metadata.json') -Encoding utf8
$taskConfig=Join-Path $taskEvidence 'fixture.cfg'
@('ambientOcclusion 0','brightness 0.7','blur 1.2','fov 92','headBob 0.8',
  'vsync 0','maxFps 0','musicEnabled 0') | Set-Content -LiteralPath $taskConfig -Encoding ascii
$taskConfigHash=(Get-FileHash -LiteralPath $taskConfig -Algorithm SHA256).Hash
$taskRuns=@()
for($taskRep=1;$taskRep -le $Repetitions;++$taskRep) {
    $taskModes=if($taskRep%2){@('on','off')}else{@('off','on')}
    foreach($taskMode in $taskModes) {
        Assert-Benchmark (@(Get-Process -Name Backrooms -ErrorAction SilentlyContinue).Count -eq 0) 'Another game instance is running.'
        $taskName='ao-'+$taskMode+'-'+$taskRep
        $taskArgs=@('--hidden','--presentation','--presentation-check',('ao-'+$taskMode),
            '--seed','12071998','--width','1280','--height','720','--fps-limit','0',
            '--frames','1000000','--run-seconds',"$($WarmupSeconds+$MeasureSeconds+5)",
            '--settings-file',($taskRelative+'/fixture.cfg'),
            '--profile',($taskRelative+'/'+$taskName+'.csv'),
            '--performance-report',($taskRelative+'/'+$taskName+'.json'))
        Write-Output "RUN $taskName ($WarmupSeconds s warmup + $MeasureSeconds s measured)"
        $taskWatch=[Diagnostics.Stopwatch]::StartNew()
        $taskProcess=Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -ArgumentList $taskArgs -WindowStyle Hidden -PassThru `
            -RedirectStandardOutput (Join-Path $taskEvidence ($taskName+'-stdout.txt')) `
            -RedirectStandardError (Join-Path $taskEvidence ($taskName+'-stderr.txt'))
        while(!$taskProcess.WaitForExit(1000)) {
            if($taskWatch.Elapsed.TotalSeconds -gt $WarmupSeconds+$MeasureSeconds+90) {
                $taskProcess.Kill();$taskProcess.WaitForExit()
                throw "Timed out: $taskName"
            }
        }
        $taskWatch.Stop()
        $taskLog=Get-Content -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Raw
        Copy-Item -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Destination (Join-Path $taskEvidence ($taskName+'.log'))
        Assert-Benchmark ($taskProcess.ExitCode -eq 0) "$taskName failed: $taskLog"
        Assert-Benchmark ((Get-Item -LiteralPath (Join-Path $taskEvidence ($taskName+'-stderr.txt'))).Length -eq 0) "$taskName wrote stderr."
        foreach($taskExpected in @('GL errors: 0','GL resources after Game: 0','Presentation: active',('Presentation verified: ao-'+$taskMode))) {
            Assert-Benchmark ($taskLog.Contains($taskExpected)) "$taskName missing $taskExpected"
        }
        $taskAo=[regex]::Match($taskLog,'Demo controls: AO amount ([0-9.]+) / AO passes ([0-9]+)')
        $taskCompleted=[regex]::Match($taskLog,'Completed ([0-9]+) frames')
        Assert-Benchmark ($taskAo.Success -and $taskCompleted.Success) "$taskName missing AO/frame counters."
        $taskAmount=Number $taskAo.Groups[1].Value
        $taskPasses=[int]$taskAo.Groups[2].Value
        $taskFrames=[int]$taskCompleted.Groups[1].Value
        if($taskMode -eq 'on') {
            Assert-Benchmark ($taskAmount -eq .85 -and $taskPasses -eq $taskFrames) "$taskName did not render AO every frame."
        } else {
            Assert-Benchmark ($taskAmount -eq 0 -and $taskPasses -eq 3) "$taskName did not disable AO after the initial toggle."
        }
        $taskReport=Get-Content -LiteralPath (Join-Path $taskEvidence ($taskName+'.json')) -Raw | ConvertFrom-Json
        Assert-Benchmark (@($taskReport.final_alive | Where-Object {$_ -ne 0}).Count -eq 0) "$taskName leaked GL objects."
        $taskRows=@(Import-Csv -LiteralPath (Join-Path $taskEvidence ($taskName+'.csv')))
        Assert-Benchmark ($taskRows.Count -eq $taskFrames) "$taskName CSV frame count mismatch."
        $taskTimes=[Collections.Generic.List[double]]::new()
        $taskElapsed=0.0;$taskMeasured=0.0;$taskFirst=0;$taskLast=0;$taskDrawn=$null
        foreach($taskRow in $taskRows) {
            $taskMs=Number $taskRow.frame_ms
            if($taskElapsed -ge $WarmupSeconds*1000 -and $taskMeasured -lt $MeasureSeconds*1000) {
                Assert-Benchmark ($taskMs -gt 0) "$taskName invalid frame time."
                Assert-Benchmark ((Number $taskRow.x) -eq 3.75 -and (Number $taskRow.z) -eq 3.75 -and (Number $taskRow.fov) -eq 78) "$taskName camera changed."
                Assert-Benchmark ([int]$taskRow.uploads -eq 0) "$taskName streamed meshes during the measured window."
                if(!$taskFirst){$taskFirst=[int]$taskRow.frame;$taskDrawn=$taskRow.drawn_triangles;$taskLook=@($taskRow.look_x,$taskRow.look_y,$taskRow.look_z)-join ','}
                Assert-Benchmark ($taskDrawn -eq $taskRow.drawn_triangles -and $taskLook -eq (@($taskRow.look_x,$taskRow.look_y,$taskRow.look_z)-join ',')) "$taskName view/geometry changed."
                $taskTimes.Add($taskMs);$taskMeasured+=$taskMs;$taskLast=[int]$taskRow.frame
            }
            $taskElapsed+=$taskMs
        }
        Assert-Benchmark ($taskMeasured -ge $MeasureSeconds*1000) "$taskName did not provide a full measurement window."
        $taskSorted=@($taskTimes | Sort-Object)
        $taskCount=$taskTimes.Count
        $taskMean=$taskMeasured/$taskCount
        $taskOver30=@($taskTimes | Where-Object {$_ -gt 1000.0/30}).Count
        $taskResult=[pscustomobject][ordered]@{
            name=$taskName;mode=$taskMode;repetition=$taskRep;order=$taskRuns.Count+1;args=$taskArgs
            processSeconds=[Math]::Round($taskWatch.Elapsed.TotalSeconds,4)
            renderer=[regex]::Match($taskLog,'Renderer: ([^\r\n]+)').Groups[1].Value
            openGL=[regex]::Match($taskLog,'OpenGL: ([^\r\n]+)').Groups[1].Value
            aoAmount=$taskAmount;aoPasses=$taskPasses;totalFrames=$taskFrames
            firstMeasuredFrame=$taskFirst;lastMeasuredFrame=$taskLast;measuredFrames=$taskCount
            measuredSeconds=$taskMeasured/1000;averageFps=1000/$taskMean;meanFrameMs=$taskMean
            p95FrameMs=$taskSorted[[int][Math]::Ceiling($taskCount*.95)-1]
            p99FrameMs=$taskSorted[[int][Math]::Ceiling($taskCount*.99)-1]
            maxFrameMs=$taskSorted[-1];framesOver33_33Ms=$taskOver30;percentFramesOver33_33Ms=100.0*$taskOver30/$taskCount
            drawnTriangles=[int]$taskDrawn;look=$taskLook;glErrors=0;glObjectsAfterShutdown=0
        }
        $taskRuns+=$taskResult
        $taskRuns | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $taskEvidence 'runs.json') -Encoding utf8
        Write-Output ('DONE {0}: {1:N2} FPS; p95 {2:N2} ms; p99 {3:N2} ms' -f $taskName,$taskResult.averageFps,$taskResult.p95FrameMs,$taskResult.p99FrameMs)
    }
}
Assert-Benchmark ((Settings-Hash) -eq $taskSettingsBefore) 'Normal settings.cfg changed during benchmark.'
Assert-Benchmark ((Get-FileHash -LiteralPath $taskConfig -Algorithm SHA256).Hash -eq $taskConfigHash) 'Benchmark fixture changed.'
Assert-Benchmark (@($taskRuns.renderer | Select-Object -Unique).Count -eq 1) 'Renderer changed between runs.'
Assert-Benchmark (@($taskRuns.drawnTriangles | Select-Object -Unique).Count -eq 1 -and @($taskRuns.look | Select-Object -Unique).Count -eq 1) 'Scene/view differed between runs.'
$taskSummary=@()
foreach($taskMode in @('on','off')) {
    $taskGroup=@($taskRuns | Where-Object {$_.mode -eq $taskMode})
    $taskRange=$taskGroup.averageFps | Measure-Object -Minimum -Maximum
    $taskSummary+=[pscustomobject][ordered]@{
        mode=$taskMode;runs=$taskGroup.Count;medianAverageFps=Median $taskGroup.averageFps
        minAverageFps=$taskRange.Minimum;maxAverageFps=$taskRange.Maximum
        medianMeanFrameMs=Median $taskGroup.meanFrameMs;medianP95FrameMs=Median $taskGroup.p95FrameMs
        medianP99FrameMs=Median $taskGroup.p99FrameMs
        measuredFrames=($taskGroup.measuredFrames | Measure-Object -Sum).Sum
        measuredSeconds=($taskGroup.measuredSeconds | Measure-Object -Sum).Sum
        framesOver33_33Ms=($taskGroup.framesOver33_33Ms | Measure-Object -Sum).Sum
    }
}
$taskMetadata.hardware.batteryAfter=Get-CimInstance Win32_Battery | Select-Object BatteryStatus,EstimatedChargeRemaining
$taskResult=[ordered]@{
    schema=1;completedAt=(Get-Date).ToString('o');metadata=$taskMetadata;summary=$taskSummary;runs=$taskRuns
    fpsGainWithAoOffPercent=100*($taskSummary[1].medianAverageFps/$taskSummary[0].medianAverageFps-1)
    normalSettingsUnchanged=$true;evidenceDirectory=$taskRelative
}
$taskResult | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding utf8
Write-Output ('EVIDENCE '+$taskEvidence)
$taskSummary | Format-Table mode,medianAverageFps,minAverageFps,maxAverageFps,medianP95FrameMs,medianP99FrameMs
