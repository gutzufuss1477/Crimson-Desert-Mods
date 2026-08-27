$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot

$Python = Get-Command python -ErrorAction SilentlyContinue
if (-not $Python) {
    $Python = Get-Command py -ErrorAction SilentlyContinue
}
if (-not $Python) {
    throw 'Python was not found in PATH.'
}

Push-Location $RepoRoot
try {
    if ($Python.Name -eq 'py.exe' -or $Python.Name -eq 'py') {
        & $Python.Source -3 -m pytest -q
    }
    else {
        & $Python.Source -m pytest -q
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Tests failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
