[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
if (-not (Get-Command 'winget' -ErrorAction SilentlyContinue)) {
    throw 'WinGet is not available. Install or repair Microsoft App Installer first; see SETUP_WINDOWS_DE.md.'
}

$packages = @(
    @{ Id = 'Git.Git'; Command = 'git' },
    @{ Id = 'GitHub.cli'; Command = 'gh' },
    @{ Id = 'Python.Python.3.13'; Command = 'py' }
)

foreach ($package in $packages) {
    if (Get-Command $package.Command -ErrorAction SilentlyContinue) {
        Write-Host "$($package.Id) is already installed."
        continue
    }

    Write-Host "Installing $($package.Id)..."
    & winget install --id $package.Id -e --source winget --accept-source-agreements --accept-package-agreements
    if ($LASTEXITCODE -ne 0) {
        throw "Could not install $($package.Id)."
    }
}

Write-Host 'Prerequisites are installed. Close PowerShell completely and open it again before running Setup-GitHub.ps1.' -ForegroundColor Green
