[CmdletBinding(DefaultParameterSetName = 'Zip')]
param(
    [Parameter(Mandatory)][ValidatePattern('^[0-9]+\.[0-9]+(?:\.[0-9]+)?$')][string]$GameVersion,
    [Parameter(Mandatory, ParameterSetName = 'Zip')][ValidateScript({ Test-Path $_ -PathType Leaf })][string]$GameZip,
    [Parameter(Mandatory, ParameterSetName = 'Folder')][ValidateScript({ Test-Path $_ -PathType Container })][string]$GameRoot,
    [string]$BuiltOutput,
    [string]$PreviousRecipeVersion = '1.16.01',
    [Parameter(Mandatory)][switch]$ConfirmedInGame,
    [switch]$CommitAndPush,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
if (-not $ConfirmedInGame) {
    throw 'A new baseline may only be captured after all generated mods were tested in game.'
}

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Python = Join-Path $RepoRoot '.venv\Scripts\python.exe'
if (-not (Test-Path $Python)) {
    throw 'The local Python environment is missing. Run scripts\Update-Mods.ps1 first.'
}
if (-not $BuiltOutput) {
    $BuiltOutput = Join-Path $RepoRoot "dist\$GameVersion"
}

$arguments = @(
    '-m', 'crimson_mod_tools', 'capture-baseline',
    '--game-version', $GameVersion,
    '--built-output', $BuiltOutput,
    '--previous-recipe-dir', (Join-Path $RepoRoot "recipes\$PreviousRecipeVersion"),
    '--destination', (Join-Path $RepoRoot "recipes\$GameVersion")
)
if ($Force) {
    $arguments += '--force'
}
if ($PSCmdlet.ParameterSetName -eq 'Zip') {
    $arguments += @('--game-zip', (Resolve-Path $GameZip).Path)
} else {
    $arguments += @('--game-root', (Resolve-Path $GameRoot).Path)
}

& $Python @arguments
if ($LASTEXITCODE -ne 0) {
    throw 'Could not capture the new baseline recipes.'
}

if ($CommitAndPush) {
    Set-Location $RepoRoot
    & git add "recipes/$GameVersion"
    & git commit -m "Add tested Crimson Desert $GameVersion baseline"
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not commit the new baseline.'
    }
    & git push
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not push the new baseline.'
    }
}

Write-Host "New tested recipe baseline created: recipes\$GameVersion" -ForegroundColor Green
