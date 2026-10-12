param(
  [string]$Binary = ''
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if ([string]::IsNullOrWhiteSpace($Binary)) {
  $Binary = Join-Path $repo 'build\windows\backend\omdrop-windows-backend.exe'
}
$state = Join-Path $env:TEMP 'omdrop-windows-backend-test.state'
$stop = Join-Path $env:TEMP 'omdrop-windows-backend-test.stop'
Remove-Item $state -ErrorAction SilentlyContinue
Remove-Item $stop -ErrorAction SilentlyContinue
$env:OMDROP_WINDOWS_STATE = $state
$env:OMDROP_WINDOWS_STOP = $stop
$env:OMDROP_WINDOWS_SIMULATED = '1'

& $binary probe --json | ConvertFrom-Json | ForEach-Object {
  if (-not $_.hardware -or $_.contract -ne '0.8.1') { throw 'probe mismatch' }
}
& $binary start 60
if ($LASTEXITCODE -ne 0) { throw 'start failed' }
$status = & $binary status --json | ConvertFrom-Json
if (-not $status.visible -or $status.remaining -le 0) { throw 'status mismatch' }
& $binary start 0
if ($LASTEXITCODE -ne 5) { throw 'extension code mismatch' }
$status = & $binary status --json | ConvertFrom-Json
if ($null -ne $status.remaining) { throw 'untimed status mismatch' }
& $binary stop
if ($LASTEXITCODE -ne 0) { throw 'stop failed' }
$status = & $binary status --json | ConvertFrom-Json
if ($status.visible) { throw 'stop status mismatch' }
Remove-Item $state -ErrorAction SilentlyContinue
Remove-Item $stop -ErrorAction SilentlyContinue
