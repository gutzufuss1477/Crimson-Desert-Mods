param(
    [Parameter(Mandatory = $true)]
    [string]$GameZip
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot

$Python = Get-Command python -ErrorAction SilentlyContinue
if (-not $Python) {
    $Python = Get-Command py -ErrorAction SilentlyContinue
}
if (-not $Python) {
    throw 'Python 3.11 or newer was not found in PATH.'
}

if (-not (Test-Path -LiteralPath $GameZip)) {
    throw "Game ZIP not found: $GameZip"
}

$env:PYTHONPATH = Join-Path $RepoRoot 'src'
if ($Python.Name -eq 'py.exe' -or $Python.Name -eq 'py') {
    & $Python.Source -3 -m crimson_mod_tools.dmm_builder --repo-root $RepoRoot --game-zip $GameZip
}
else {
    & $Python.Source -m crimson_mod_tools.dmm_builder --repo-root $RepoRoot --game-zip $GameZip
}

if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE"
}

Write-Host "Build complete: $RepoRoot\dist\2.00.00\packages" -ForegroundColor Green
