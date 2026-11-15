param([string]$Executable='')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
if(!$Executable){$Executable=Join-Path $taskRoot 'Backrooms.exe'}
$taskExe=(Resolve-Path -LiteralPath $Executable).Path
$taskRuntime=Split-Path -Parent $taskExe

# The PE subsystem is what makes an Explorer/double-click launch receive a
# visible Windows console. Redirected tests below exercise the same executable.
$taskImage=[IO.File]::ReadAllBytes($taskExe)
if($taskImage.Length -lt 64){throw 'Executable has no DOS/PE header.'}
$taskPeOffset=[BitConverter]::ToInt32($taskImage,60)
if($taskPeOffset -lt 0 -or $taskPeOffset+94 -gt $taskImage.Length -or
   [BitConverter]::ToUInt32($taskImage,$taskPeOffset) -ne 0x4550){throw 'Invalid PE header.'}
$taskOptionalOffset=$taskPeOffset+24
$taskMagic=[BitConverter]::ToUInt16($taskImage,$taskOptionalOffset)
if($taskMagic -ne 0x10b -and $taskMagic -ne 0x20b){throw 'Unsupported PE optional header.'}
$taskSubsystem=[BitConverter]::ToUInt16($taskImage,$taskOptionalOffset+68)
if($taskSubsystem -ne 3){throw "Expected Windows console subsystem 3; found $taskSubsystem. Rebuild the game first."}

$taskEvidence=Join-Path $taskRoot ('tests/artifacts/console/run-'+[Guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $taskEvidence -Force | Out-Null
$taskResults=@([pscustomobject]@{name='windows-console-subsystem';passed=$true;subsystem=$taskSubsystem})
Write-Output 'PASS windows-console-subsystem'

# Each case has its own executable and log. No original shader, texture, user
# settings or shared backrooms.log is modified. Keep evidence; never delete it.
# Failure cases stop before the creature model is loaded, so large asset copies
# are unnecessary. A working OpenGL 3.3 driver is needed for the graphics cases.
$taskShaders=@('vshader_backrooms.glsl','fshader_backrooms.glsl','vshader_lens.glsl',
    'fshader_lens.glsl','vshader_shadow.glsl','fshader_shadow.glsl','fshader_ao.glsl')
$taskCases=@(
    @{name='help';args=@('--hidden','--help');code=0;output='Backrooms: --seed N';error=''},
    @{name='invalid-option';args=@('--hidden','--console-invalid-option');code=1;output='';error='Unknown option: --console-invalid-option'},
    @{name='missing-shader';args=@('--hidden','--frames','1');code=1;output='OpenGL:';error='Cannot read shader: fshader_backrooms.glsl'},
    @{name='malformed-shader';args=@('--hidden','--frames','1');code=1;output='OpenGL:';error='Shader compile failed'},
    @{name='missing-texture';args=@('--hidden','--frames','1');code=1;output='OpenGL:';error='Cannot load creature texture assets/models/aranea/creature-color.rgb'}
)
foreach($taskCase in $taskCases) {
    $taskCaseDir=Join-Path $taskEvidence $taskCase.name
    New-Item -ItemType Directory -Path $taskCaseDir | Out-Null
    $taskCaseExe=Join-Path $taskCaseDir 'Backrooms.exe'
    Copy-Item -LiteralPath $taskExe -Destination $taskCaseExe
    foreach($taskShader in $taskShaders) {
        if($taskCase.name -eq 'missing-shader' -and $taskShader -eq 'fshader_backrooms.glsl'){continue}
        Copy-Item -LiteralPath (Join-Path $taskRuntime $taskShader) -Destination (Join-Path $taskCaseDir $taskShader)
    }
    foreach($taskConfig in @('balance.cfg','jumpscare.cfg')) {
        Copy-Item -LiteralPath (Join-Path $taskRuntime $taskConfig) -Destination (Join-Path $taskCaseDir $taskConfig)
    }
    if($taskCase.name -eq 'malformed-shader') {
        @('#version 330 core','this is deliberately invalid GLSL') |
            Set-Content -LiteralPath (Join-Path $taskCaseDir 'fshader_backrooms.glsl') -Encoding ascii
    }
    $taskStdout=Join-Path $taskCaseDir 'stdout.txt'
    $taskStderr=Join-Path $taskCaseDir 'stderr.txt'
    $taskProcess=Start-Process -FilePath $taskCaseExe -WorkingDirectory $taskCaseDir -ArgumentList $taskCase.args `
        -WindowStyle Hidden -RedirectStandardOutput $taskStdout -RedirectStandardError $taskStderr -PassThru
    $taskProcessHandle=$taskProcess.Handle
    if(!$taskProcess.WaitForExit(60000)) {
        $taskProcess.Kill()
        throw "Console verification timed out: $($taskCase.name)"
    }
    # Wait for redirected output handlers to finish draining after process exit.
    $taskProcess.WaitForExit()
    $taskProcess.Refresh()
    $taskOut=[IO.File]::ReadAllText($taskStdout)
    $taskErr=[IO.File]::ReadAllText($taskStderr)
    $taskLog=[IO.File]::ReadAllText((Join-Path $taskCaseDir 'backrooms.log'))
    if($taskProcess.ExitCode -ne $taskCase.code) {
        throw "$($taskCase.name): expected exit $($taskCase.code), got $($taskProcess.ExitCode).`n$taskOut`n$taskErr"
    }
    if($taskCase.output -and !$taskOut.Contains($taskCase.output)) {
        throw "$($taskCase.name): stdout does not contain '$($taskCase.output)'.`n$taskOut"
    }
    if($taskCase.error) {
        if(!$taskErr.Contains('ERROR: ') -or !$taskErr.Contains($taskCase.error) -or !$taskLog.Contains($taskCase.error)) {
            throw "$($taskCase.name): diagnostic was not written to both stderr and backrooms.log.`n$taskErr`n$taskLog"
        }
        if($taskCase.name -eq 'malformed-shader' -and !$taskErr.Contains('fshader_backrooms.glsl')) {
            throw 'Shader compile diagnostic must identify the failing file.'
        }
    } elseif($taskErr.Length -ne 0) {
        throw "$($taskCase.name): unexpected stderr output.`n$taskErr"
    }
    $taskResults += [pscustomobject]@{name=$taskCase.name;passed=$true;exitCode=$taskProcess.ExitCode;
        stdout=$taskStdout;stderr=$taskStderr;log=(Join-Path $taskCaseDir 'backrooms.log')}
    Write-Output "PASS $($taskCase.name)"
}
$taskResults | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding utf8
Write-Output "Evidence: $taskEvidence"
