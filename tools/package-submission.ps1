param(
    [string]$OutputDirectory='dist',
    [string]$Executable='',
    [string[]]$Deliverables=@(),
    [switch]$SourceOnly
)
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
if(![IO.Path]::IsPathRooted($OutputDirectory)){$OutputDirectory=Join-Path $taskRoot $OutputDirectory}
$taskOutput=[IO.Path]::GetFullPath($OutputDirectory)
foreach($taskIncludedFolder in @('app','common','include','tools','docs','.github','tests','assets')) {
    $taskIncludedPath=[IO.Path]::GetFullPath((Join-Path $taskRoot $taskIncludedFolder))
    if($taskOutput.Equals($taskIncludedPath,[StringComparison]::OrdinalIgnoreCase) -or
       $taskOutput.StartsWith($taskIncludedPath+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {
        throw 'Choose an output directory outside the source and asset folders, such as dist or build/submission-input.'
    }
}
$taskKind=if($SourceOnly){'source'}else{'windows'}
$taskStamp=Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$taskName='BackroomsCGF-'+$taskKind+'-'+$taskStamp
$taskStageParent=Join-Path $taskOutput ($taskName+'-stage')
$taskStage=Join-Path $taskStageParent 'BackroomsCGF'
$taskZip=Join-Path $taskOutput ($taskName+'.zip')
if((Test-Path -LiteralPath $taskStageParent) -or (Test-Path -LiteralPath $taskZip)) {
    throw 'This staging directory or ZIP already exists. Choose a fresh output directory or retry with a new timestamp.'
}
if($SourceOnly -and $Executable){throw 'Choose either -SourceOnly or -Executable.'}
if(!$SourceOnly) {
    if(!$Executable){$Executable=Join-Path $taskRoot 'Backrooms.exe'}
    if(![IO.Path]::IsPathRooted($Executable)){$Executable=Join-Path $taskRoot $Executable}
    $Executable=(Resolve-Path -LiteralPath $Executable).Path
    if(!(Test-Path -LiteralPath $Executable -PathType Leaf)){throw 'The selected executable is not a file.'}
}
function Get-PackageFiles([string]$Directory) {
    $pending=[Collections.Generic.Stack[string]]::new()
    $pending.Push($Directory)
    while($pending.Count) {
        foreach($item in (Get-ChildItem -LiteralPath $pending.Pop() -Force)) {
            if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Refusing linked input: $($item.FullName)"}
            if($item.PSIsContainer){$pending.Push($item.FullName)}else{$item}
        }
    }
}
function Copy-PackageFile([string]$InputFile,[string]$Relative) {
    $destination=[IO.Path]::GetFullPath((Join-Path $taskStage $Relative))
    if(!$destination.StartsWith($taskStage+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {
        throw "Package path escapes its staging directory: $Relative"
    }
    if(Test-Path -LiteralPath $destination){throw "Duplicate package path: $Relative"}
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $InputFile -Destination $destination
}
$taskRequiredSource=@('CMakeLists.txt','build.ps1','README.md','README_FIRST.txt','THIRD_PARTY.md',
    'Run.cmd','Presentation.cmd','Lab Check.cmd','assets/asset-license-manifest.json',
    'tools/verify-submission.ps1','tools/benchmark-ao.ps1')
foreach($taskRelative in $taskRequiredSource) {
    if(!(Test-Path -LiteralPath (Join-Path $taskRoot $taskRelative) -PathType Leaf)){throw "Required package source is absent: $taskRelative"}
}
# Select source/development files explicitly. No Git history, build tree,
# backups, saved settings, logs or previous submission artifacts are traversed.
$taskRootExtensions=@('.c','.cpp','.h','.hpp','.glsl','.cfg','.ps1','.cmd')
$taskRootNames=@('CMakeLists.txt','CMakePresets.json','Makefile','README.md','README_FIRST.txt','THIRD_PARTY.md')
$taskTextExtensions=@('.c','.cpp','.h','.hpp','.glsl','.py','.ps1','.json','.md','.txt','.cmake','.in','.yml','.yaml')
New-Item -ItemType Directory -Path $taskStage | Out-Null
foreach($taskFile in (Get-ChildItem -LiteralPath $taskRoot -File -Force)) {
    if($taskFile.Name -eq 'settings.cfg'){continue}
    if($taskFile.Name -in $taskRootNames -or $taskFile.Extension -in $taskRootExtensions) {
        if($taskFile.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Refusing linked input: $($taskFile.FullName)"}
        Copy-PackageFile $taskFile.FullName $taskFile.Name
    }
}
foreach($taskFolder in @('app','common','include','tools','docs','.github')) {
    $taskBase=Join-Path $taskRoot $taskFolder
    if(!(Test-Path -LiteralPath $taskBase -PathType Container)){continue}
    foreach($taskFile in (Get-PackageFiles $taskBase)) {
        $taskRelative=$taskFile.FullName.Substring($taskRoot.Length+1)
        if($taskRelative -match '(?i)(^|[\\/])(__pycache__|\.qa|artifacts|node_modules)([\\/]|$)'){continue}
        if($taskFile.Name -in @('report-draft.md','presentation.html')){continue}
        if($taskFile.Extension -notin $taskTextExtensions){continue}
        Copy-PackageFile $taskFile.FullName $taskRelative
    }
}
# Include only test source, never the development workspace's captures/binaries.
foreach($taskFile in (Get-ChildItem -LiteralPath (Join-Path $taskRoot 'tests') -File)) {
    if($taskFile.Extension -in @('.cpp','.c','.h','.hpp')){Copy-PackageFile $taskFile.FullName ('tests/'+$taskFile.Name)}
}
foreach($taskFile in (Get-PackageFiles (Join-Path $taskRoot 'assets'))) {
    if($taskFile.Name -eq '.DS_Store' -or $taskFile.FullName -match '(?i)[\\/]__pycache__[\\/]'){continue}
    Copy-PackageFile $taskFile.FullName $taskFile.FullName.Substring($taskRoot.Length+1)
}
if(!$SourceOnly){Copy-PackageFile $Executable 'Backrooms.exe'}
$taskIncludedDeliverables=@()
foreach($taskInput in $Deliverables) {
    $taskFile=Get-Item -LiteralPath $taskInput
    if($taskFile.PSIsContainer){throw "Deliverables must be files: $taskInput"}
    if($taskFile.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Refusing linked deliverable: $taskInput"}
    Copy-PackageFile $taskFile.FullName ('Deliverables/'+$taskFile.Name)
    $taskIncludedDeliverables+=('Deliverables/'+$taskFile.Name)
}
$taskFiles=@(Get-PackageFiles $taskStage | Sort-Object FullName | ForEach-Object {
    [pscustomobject][ordered]@{path=$_.FullName.Substring($taskStage.Length+1).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
})
$taskCommit=$null
if(Get-Command git -ErrorAction SilentlyContinue) {
    $taskGitValue=& git -C $taskRoot rev-parse HEAD 2>$null
    if($LASTEXITCODE -eq 0){$taskCommit=($taskGitValue | Out-String).Trim()}
}
$taskManifest=[ordered]@{schema=1;package='BackroomsCGF';createdUtc=(Get-Date).ToUniversalTime().ToString('o');sourceOnly=[bool]$SourceOnly;sourceCommit=$taskCommit;deliverables=$taskIncludedDeliverables;files=$taskFiles}
$taskManifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $taskStage 'package-manifest.json') -Encoding utf8
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($taskStage,$taskZip,[IO.Compression.CompressionLevel]::Optimal,$true)
$taskResult=[pscustomobject][ordered]@{stagingDirectory=$taskStage;zip=$taskZip;sha256=(Get-FileHash -LiteralPath $taskZip -Algorithm SHA256).Hash.ToLowerInvariant();zipBytes=(Get-Item -LiteralPath $taskZip).Length;fileCount=$taskFiles.Count+1;sourceOnly=[bool]$SourceOnly;deliverables=$taskIncludedDeliverables}
$taskResult | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskOutput ($taskName+'.package.json')) -Encoding utf8
Write-Output ('PACKAGE '+$taskZip)
Write-Output ('SHA256 '+$taskResult.sha256)
Write-Output ('STAGING '+$taskStage)
Write-Output ('FILES '+$taskResult.fileCount)
