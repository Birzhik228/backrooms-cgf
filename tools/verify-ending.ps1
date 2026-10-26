param([string]$CaseFilter='*')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $taskRoot
$taskEvidence=Join-Path $taskRoot 'tests\artifacts\ending'
New-Item -ItemType Directory -Force -Path $taskEvidence | Out-Null
$taskCases=@(
 @{name='cabin';args='--ending-time 1.5 --frames 2';expected=@('Ending started: preview','Ending phase: Doors closing')},
 @{name='ascent';args='--ending-time 7 --frames 2';expected=@('Ending started: preview','Ending phase: Ascending')},
 @{name='arrival';args='--ending-time 12.8 --frames 2';expected=@('Ending started: preview','Ending phase: Surface')},
 @{name='lobby';args='--ending-time 18 --frames 2';expected=@('Ending started: preview','Ending phase: Lobby')},
 @{name='door';args='--ending-time 22 --frames 2';expected=@('Ending started: preview','Ending phase: Exit opening')},
 @{name='reveal';args='--ending-time 26 --frames 2';expected=@('Ending started: preview','Ending phase: City street')},
 @{name='city-life';args='--ending-time 32 --frames 2';expected=@('Ending started: preview','Ending phase: City life','Ending city: 12 walking pedestrians / 6 moving cars.')},
 @{name='city-late';args='--ending-time 37 --frames 2';expected=@('Ending started: preview','Ending phase: City life','Ending city: 12 walking pedestrians / 6 moving cars.')},
 @{name='result';args='--ending-time 42 --frames 2';expected=@('Ending started: preview','Ending finished: You found the way out.')},
 @{name='full-sequence';args='--ending-preview --frames 2540';expected=@('Ending phase: Doors closing','Ending phase: Ascending','Ending phase: Surface','Ending phase: Lobby','Ending phase: Exit opening','Ending phase: City street','Ending phase: City life','Ending phase: Fade','Ending finished: You found the way out.')},
 @{name='controls';args='--ending-check --frames 45';expected=@('Ending paused.','Ending resumed.','Ending sound: off','Ending sound: on','Ending finished: You found the way out.','Ending replayed.','Ending returned to menu.','/ active no / completed 2 / replayed 1')},
 @{name='city-controls';args='--ending-check --ending-time 32 --frames 45';expected=@('Ending phase: City life','Ending paused.','Ending resumed.','Ending sound: off','Ending sound: on','Ending finished: You found the way out.','Ending replayed.','Ending returned to menu.','/ active no / completed 2 / replayed 1');counts=@{'Ending ambience restored: city (unmute)'=2}},
 @{name='parking-exit';args='--parking --auto-breakers --auto-exit --auto-walk --no-threat --frames 130';expected=@('Parking completed. All three circuits restored; service exit reached.','Ending started: parking exit','Circuits: 3/3','Catches: 0')}
)
$taskResults=@()
foreach($taskCase in $taskCases) {
 if($taskCase.name -notlike $CaseFilter){continue}
 $taskImage='tests/artifacts/ending/'+$taskCase.name+'.bmp'
 $taskClock=[Diagnostics.Stopwatch]::StartNew()
 $taskProcess=Start-Process -FilePath (Join-Path $taskRoot 'Backrooms.exe') -WorkingDirectory $taskRoot -ArgumentList ('--hidden --demo --seed 42 --width 1280 --height 800 --fps-limit 0 '+$taskCase.args+' --capture '+$taskImage) -WindowStyle Hidden -PassThru
 if(!$taskProcess.WaitForExit(60000)) {
  # Stop only this verification process, never a separately opened game.
  $taskProcess.Kill();$taskProcess.WaitForExit()
  throw "Ending case timed out: $($taskCase.name)"
 }
 $taskClock.Stop()
 $taskLog=Get-Content -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Raw
 Copy-Item -LiteralPath (Join-Path $taskRoot 'backrooms.log') -Destination (Join-Path $taskEvidence ($taskCase.name+'.log'))
 if($taskProcess.ExitCode -ne 0 -or !$taskLog.Contains('GL errors: 0')){throw "$($taskCase.name): game failed`n$taskLog"}
 foreach($taskExpected in (@('Ending gameplay frozen: yes')+$taskCase.expected)) {
  if(!$taskLog.Contains($taskExpected)){throw "$($taskCase.name): missing $taskExpected`n$taskLog"}
 }
 if($taskCase.counts) { foreach($taskCount in $taskCase.counts.GetEnumerator()) {
  if([regex]::Matches($taskLog,[regex]::Escape($taskCount.Key)).Count -ne $taskCount.Value){throw "$($taskCase.name): expected $($taskCount.Value) occurrences of $($taskCount.Key)`n$taskLog"}
 } }
 if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).Length -lt 1024){throw "$($taskCase.name): capture missing or empty"}
 $taskResults+=[pscustomobject]@{case=$taskCase.name;passed=$true;wallSeconds=[Math]::Round($taskClock.Elapsed.TotalSeconds,3);capture=$taskImage}
 Write-Output "PASS $($taskCase.name)"
}
if(!$taskResults.Count){throw "No ending cases match $CaseFilter"}
$taskResults | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $taskEvidence 'results.json') -Encoding UTF8
