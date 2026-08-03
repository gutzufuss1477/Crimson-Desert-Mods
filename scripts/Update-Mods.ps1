[CmdletBinding(DefaultParameterSetName = 'Zip')]
param(
    [Parameter(Mandatory)][ValidatePattern('^[0-9]+\.[0-9]+(?:\.[0-9]+)?$')][string]$GameVersion,
    [Parameter(Mandatory, ParameterSetName = 'Zip')][ValidateScript({ Test-Path $_ -PathType Leaf })][string]$GameZip,
    [Parameter(Mandatory, ParameterSetName = 'Folder')][ValidateScript({ Test-Path $_ -PathType Container })][string]$GameRoot,
    [string]$RecipeVersion = '1.16.01',
    [string]$OutputDirectory,
    [switch]$OpenOutput
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$VenvRoot = Join-Path $RepoRoot '.venv'
$VenvPython = Join-Path $VenvRoot 'Scripts\python.exe'

function Invoke-Checked {
    param([Parameter(Mandatory)][scriptblock]$Command, [Parameter(Mandatory)][string]$FailureMessage)
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw $FailureMessage
    }
}

if (-not (Test-Path $VenvPython)) {
    if (Get-Command 'py' -ErrorAction SilentlyContinue) {
        Invoke-Checked -Command { & py -3 -m venv $VenvRoot } -FailureMessage 'Could not create the Python virtual environment.'
    } elseif (Get-Command 'python' -ErrorAction SilentlyContinue) {
        Invoke-Checked -Command { & python -m venv $VenvRoot } -FailureMessage 'Could not create the Python virtual environment.'
    } else {
        throw 'Python 3 is not installed. See SETUP_WINDOWS_DE.md.'
    }
}

Invoke-Checked -Command { & $VenvPython -m pip install --disable-pip-version-check --no-build-isolation -e $RepoRoot } -FailureMessage 'Could not install the local build tool.'

$RecipeDirectory = Join-Path $RepoRoot "recipes\$RecipeVersion"
if (-not (Test-Path $RecipeDirectory -PathType Container)) {
    throw "Recipe version not found: $RecipeDirectory"
}

if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $RepoRoot "dist\$GameVersion"
}

$arguments = @(
    '-m', 'crimson_mod_tools', 'build',
    '--game-version', $GameVersion,
    '--recipe-dir', $RecipeDirectory,
    '--output', $OutputDirectory
)
if ($PSCmdlet.ParameterSetName -eq 'Zip') {
    $arguments += @('--game-zip', (Resolve-Path $GameZip).Path)
} else {
    $arguments += @('--game-root', (Resolve-Path $GameRoot).Path)
}

& $VenvPython @arguments
$buildExitCode = $LASTEXITCODE
$reportPath = Join-Path $OutputDirectory 'BUILD_REPORT.json'
if ($buildExitCode -ne 0) {
    Write-Host "The build completed with one or more blocked mods. Review: $reportPath" -ForegroundColor Yellow
    exit $buildExitCode
}

Write-Host "All supported packages were built and validated: $OutputDirectory" -ForegroundColor Green
if ($OpenOutput) {
    Start-Process explorer.exe $OutputDirectory
}
