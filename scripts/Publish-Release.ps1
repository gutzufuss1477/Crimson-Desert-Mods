[CmdletBinding()]
param(
    [string]$Owner = 'gutzufuss1477',
    [string]$Repository = 'Crimson-Desert-Mods',
    [Parameter(Mandatory)][ValidatePattern('^[0-9]+\.[0-9]+(?:\.[0-9]+)?$')][string]$Version,
    [string]$AssetDirectory,
    [switch]$Draft
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

function Invoke-NativeQuiet {
    param(
        [Parameter(Mandatory)][string]$Command,
        [string[]]$Arguments = @()
    )

    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Command @Arguments 1>$null 2>$null
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousPreference
    }
    return $exitCode
}

function Invoke-NativeChecked {
    param(
        [Parameter(Mandatory)][string]$Command,
        [string[]]$Arguments = @(),
        [Parameter(Mandatory)][string]$FailureMessage
    )

    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Command @Arguments
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousPreference
    }

    if ($exitCode -ne 0) {
        throw $FailureMessage
    }
}

if (-not (Get-Command 'gh' -ErrorAction SilentlyContinue)) {
    throw 'GitHub CLI (gh) is not installed.'
}

if ((Invoke-NativeQuiet -Command 'gh' -Arguments @('auth', 'status')) -ne 0) {
    throw 'GitHub CLI is not authenticated. Run: gh auth login'
}

if (-not $AssetDirectory) {
    $AssetDirectory = Join-Path $RepoRoot "dist\$Version"
}
if (-not (Test-Path $AssetDirectory -PathType Container)) {
    throw "Asset directory not found: $AssetDirectory"
}

$assets = Get-ChildItem -Path $AssetDirectory -Recurse -File | Where-Object {
    $_.Extension -eq '.zip' -or $_.Name -in @('BUILD_REPORT.json', 'SHA256SUMS.txt')
} | Sort-Object FullName
if (-not $assets) {
    throw "No release assets found below $AssetDirectory"
}

$releaseNotes = Join-Path $AssetDirectory 'RELEASE_NOTES.md'
if (-not (Test-Path $releaseNotes)) {
    $releaseText = @"
# Crimson Desert $Version

Automated semantic rebuild against clean Crimson Desert $Version files.

The release contains normal mod-manager packages, DMM Healthbar packages, a validation report, and SHA-256 checksums.

Only one Healthbar variant may be active. Use either Mount Speed or Mount All Stats, not both.
"@
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($releaseNotes, $releaseText, $utf8NoBom)
}

$repo = "$Owner/$Repository"
$tag = "game-$Version"
$releaseExists = (Invoke-NativeQuiet -Command 'gh' -Arguments @('release', 'view', $tag, '--repo', $repo)) -eq 0

if ($releaseExists) {
    $uploadArgs = @('release', 'upload', $tag, '--repo', $repo, '--clobber') + $assets.FullName
    Invoke-NativeChecked -Command 'gh' -Arguments $uploadArgs -FailureMessage 'Could not update the existing GitHub release assets.'
    Invoke-NativeChecked -Command 'gh' -Arguments @('release', 'edit', $tag, '--repo', $repo, '--title', "Crimson Desert Mods $Version", '--notes-file', $releaseNotes) -FailureMessage 'Could not update the GitHub release metadata.'
} else {
    $createArgs = @(
        'release', 'create', $tag,
        '--repo', $repo,
        '--target', 'main',
        '--title', "Crimson Desert Mods $Version",
        '--notes-file', $releaseNotes
    )
    if ($Draft) {
        $createArgs += '--draft'
    }
    $createArgs += $assets.FullName
    Invoke-NativeChecked -Command 'gh' -Arguments $createArgs -FailureMessage 'Could not create the GitHub release.'
}

Write-Host "GitHub release published: $repo / $tag" -ForegroundColor Green
