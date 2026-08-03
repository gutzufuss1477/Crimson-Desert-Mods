[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

function Get-CommandVersion {
    param([Parameter(Mandatory)][string]$Name, [string[]]$Arguments = @('--version'))
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if (-not $command) {
        return [pscustomobject]@{ Name = $Name; Installed = $false; Version = $null }
    }
    $output = & $Name @Arguments 2>&1 | Select-Object -First 1
    return [pscustomobject]@{ Name = $Name; Installed = $true; Version = [string]$output }
}

$results = @(
    Get-CommandVersion -Name 'git'
    Get-CommandVersion -Name 'gh'
)

if (Get-Command 'py' -ErrorAction SilentlyContinue) {
    $pythonOutput = & py -3 --version 2>&1 | Select-Object -First 1
    $results += [pscustomobject]@{ Name = 'python'; Installed = $LASTEXITCODE -eq 0; Version = [string]$pythonOutput }
} else {
    $results += Get-CommandVersion -Name 'python'
}

$results | Format-Table -AutoSize

$missing = $results | Where-Object { -not $_.Installed }
if ($missing) {
    throw "Missing tools: $($missing.Name -join ', '). See SETUP_WINDOWS_DE.md."
}

$pythonExe = Join-Path $RepoRoot '.venv\Scripts\python.exe'
if (Test-Path $pythonExe) {
    & $pythonExe -m crimson_mod_tools validate-recipes
    if ($LASTEXITCODE -ne 0) {
        throw 'Recipe validation failed.'
    }
} else {
    Write-Host 'Python environment is not initialized yet. Run scripts\Update-Mods.ps1 once.'
}

Write-Host 'Environment check completed.'
