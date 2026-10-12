$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Upstream = Join-Path $Root 'upstream\owl'
$Repo = 'https://github.com/jedbillyb/owl.git'
$pins = @{}
Get-Content (Join-Path $Root 'pins') |
    Where-Object { $_ -match '^\s*([A-Z_]+)=(\S+)' } |
    ForEach-Object { $pins[$Matches[1]] = $Matches[2] }

if (-not $pins.ContainsKey('OWL_COMMIT')) {
    throw 'pins does not define OWL_COMMIT'
}

if (-not (Test-Path (Join-Path $Upstream '.git'))) {
    New-Item -ItemType Directory -Force (Split-Path $Upstream) | Out-Null
    git clone --recurse-submodules $Repo $Upstream
    if ($LASTEXITCODE) { throw 'Could not clone OWL' }
}

git -C $Upstream fetch --quiet origin
if ($LASTEXITCODE) { throw 'Could not fetch OWL' }
git -C $Upstream reset --hard --quiet $pins['OWL_COMMIT']
if ($LASTEXITCODE) { throw 'Could not select pinned OWL commit' }
git -C $Upstream submodule update --init --recursive --quiet
if ($LASTEXITCODE) { throw 'Could not update OWL submodules' }

Get-ChildItem (Join-Path $Root 'patches\owl-*.patch') |
    Sort-Object Name |
    ForEach-Object {
        git -C $Upstream apply $_.FullName
        if ($LASTEXITCODE) { throw "$($_.Name) did not apply to pinned OWL" }
    }

Write-Host "Prepared OWL at $($pins['OWL_COMMIT']) with patches applied."
