param([string]$CaseFilter='*')
& (Join-Path $PSScriptRoot 'verify-aranea.ps1') -CaseFilter $CaseFilter
