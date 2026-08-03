[CmdletBinding()]
param(
    [string]$Owner = 'gutzufuss1477',
    [string]$Repository = 'Crimson-Desert-Mods',
    [ValidateSet('private', 'public', 'internal')][string]$Visibility = 'private',
    [switch]$CreateInitialRelease,
    [switch]$OpenInBrowser
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Set-Location $RepoRoot

function Invoke-NativeQuiet {
    param(
        [Parameter(Mandatory)][string]$Command,
        [string[]]$Arguments = @()
    )

    $previousPreference = $ErrorActionPreference
    try {
        # Windows PowerShell 5.1 converts native STDERR into PowerShell errors.
        # Expected failures must therefore run independently of the script-wide Stop policy.
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

function Invoke-NativeCapture {
    param(
        [Parameter(Mandatory)][string]$Command,
        [string[]]$Arguments = @()
    )

    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $output = @(& $Command @Arguments 2>$null)
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousPreference
    }

    [pscustomobject]@{
        ExitCode = $exitCode
        Output = $output
    }
}

foreach ($command in @('git', 'gh')) {
    if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
        throw "$command is not installed. See SETUP_WINDOWS_DE.md."
    }
}

$authExitCode = Invoke-NativeQuiet -Command 'gh' -Arguments @('auth', 'status')
if ($authExitCode -ne 0) {
    Write-Host 'GitHub authentication is required. A browser window will open.'
    Invoke-NativeChecked -Command 'gh' -Arguments @('auth', 'login', '--web', '--git-protocol', 'https') -FailureMessage 'GitHub authentication failed.'
}

if (-not (Test-Path (Join-Path $RepoRoot '.git'))) {
    Invoke-NativeChecked -Command 'git' -Arguments @('init', '-b', 'main') -FailureMessage 'Could not initialize the local Git repository.'
}

$gitNameResult = Invoke-NativeCapture -Command 'git' -Arguments @('config', '--get', 'user.name')
$gitEmailResult = Invoke-NativeCapture -Command 'git' -Arguments @('config', '--get', 'user.email')
$gitName = ($gitNameResult.Output | Select-Object -First 1)
$gitEmail = ($gitEmailResult.Output | Select-Object -First 1)

if (-not $gitName -or -not $gitEmail) {
    $loginResult = Invoke-NativeCapture -Command 'gh' -Arguments @('api', 'user', '--jq', '.login')
    if ($loginResult.ExitCode -ne 0 -or -not $loginResult.Output) {
        throw 'Could not determine the authenticated GitHub user.'
    }
    $login = ([string]($loginResult.Output | Select-Object -First 1)).Trim()

    if (-not $gitName) {
        Invoke-NativeChecked -Command 'git' -Arguments @('config', 'user.name', $login) -FailureMessage 'Could not configure the local Git user name.'
    }
    if (-not $gitEmail) {
        Invoke-NativeChecked -Command 'git' -Arguments @('config', 'user.email', "$login@users.noreply.github.com") -FailureMessage 'Could not configure the local Git email address.'
    }
}

Invoke-NativeChecked -Command 'git' -Arguments @('add', '--all') -FailureMessage 'Could not stage the repository files.'
$diffExitCode = Invoke-NativeQuiet -Command 'git' -Arguments @('diff', '--cached', '--quiet')
if ($diffExitCode -eq 1) {
    Invoke-NativeChecked -Command 'git' -Arguments @('commit', '-m', 'Initial Crimson Desert mod build system') -FailureMessage 'Could not create the initial Git commit.'
} elseif ($diffExitCode -ne 0) {
    throw 'Could not inspect the staged Git changes.'
}

$repo = "$Owner/$Repository"
$repositoryExists = (Invoke-NativeQuiet -Command 'gh' -Arguments @('repo', 'view', $repo, '--json', 'name')) -eq 0

if (-not $repositoryExists) {
    $createArgs = @(
        'repo', 'create', $repo,
        '--source', '.',
        '--remote', 'origin',
        '--push',
        '--description', 'Reproducible build and validation system for the Crimson Desert mod collection.',
        '--disable-wiki'
    )
    switch ($Visibility) {
        'private' { $createArgs += '--private' }
        'public' { $createArgs += '--public' }
        'internal' { $createArgs += '--internal' }
    }
    Invoke-NativeChecked -Command 'gh' -Arguments $createArgs -FailureMessage 'Could not create or push the GitHub repository.'
} else {
    # Do not call `git remote get-url origin` here: a missing remote writes to STDERR,
    # which aborts Windows PowerShell 5.1 when ErrorActionPreference is Stop.
    $remoteResult = Invoke-NativeCapture -Command 'git' -Arguments @('remote')
    if ($remoteResult.ExitCode -ne 0) {
        throw 'Could not read the configured Git remotes.'
    }
    $remoteNames = @($remoteResult.Output | ForEach-Object { ([string]$_).Trim() } | Where-Object { $_ })
    if ($remoteNames -notcontains 'origin') {
        Invoke-NativeChecked -Command 'git' -Arguments @('remote', 'add', 'origin', "https://github.com/$repo.git") -FailureMessage 'Could not add the GitHub repository as origin.'
    }
    Invoke-NativeChecked -Command 'git' -Arguments @('push', '-u', 'origin', 'main') -FailureMessage 'Could not push to the existing GitHub repository.'
}

$editExitCode = Invoke-NativeQuiet -Command 'gh' -Arguments @('repo', 'edit', $repo, '--enable-wiki=false', '--enable-issues')
if ($editExitCode -ne 0) {
    Write-Warning 'Repository settings could not be adjusted, but the push succeeded.'
}
Invoke-NativeChecked -Command 'gh' -Arguments @('repo', 'set-default', $repo) -FailureMessage 'Could not set the default GitHub repository.'

if ($CreateInitialRelease) {
    if ($Visibility -ne 'private') {
        Write-Warning 'The initial release contains mod packages with modified game-derived files. Review distribution rights before using a public repository.'
    }
    & (Join-Path $PSScriptRoot 'Publish-Release.ps1') -Owner $Owner -Repository $Repository -Version '1.16.01' -AssetDirectory (Join-Path $RepoRoot 'release-assets\1.16.01')
}

Write-Host "Repository is configured and pushed: https://github.com/$repo" -ForegroundColor Green
if ($OpenInBrowser) {
    $browserExitCode = Invoke-NativeQuiet -Command 'gh' -Arguments @('repo', 'view', $repo, '--web')
    if ($browserExitCode -ne 0) {
        Write-Warning "Could not open the browser automatically. Open https://github.com/$repo manually."
    }
}
