$ErrorActionPreference='Stop'
function File-Digest([string]$path) {
    $algorithm=[Security.Cryptography.SHA256]::Create()
    $stream=[IO.File]::OpenRead($path)
    try { return ([BitConverter]::ToString($algorithm.ComputeHash($stream))).Replace('-','') }
    finally { $stream.Dispose(); $algorithm.Dispose() }
}
$root=Join-Path $PSScriptRoot ('out\load-tests-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'out\load_tests.exe') -Destination (Join-Path $root 'CrimsonDesert.exe')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'out\load_tests.exe') -Destination $root
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'out\bank_refresh_instrumented.dll') -Destination $root
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'out\Bank_Refresh_2.03.02.asi') -Destination $root
$ini=Join-Path $root 'Bank_Refresh.ini'
$cases=@(
    @{name='missing INI'; ini=$null; result=4},
    @{name='disabled'; ini="[BankRefresh]`nEnabled=0`nIntervalGameMinutes=15"; result=5},
    @{name='missing enabled'; ini="[BankRefresh]`nIntervalGameMinutes=15"; result=5},
    @{name='default interval'; ini="[BankRefresh]`nEnabled=1`nIntervalGameMinutes=15"; result=9},
    @{name='minimum interval'; ini="[BankRefresh]`nEnabled=1`nIntervalGameMinutes=1"; result=9},
    @{name='maximum interval'; ini="[BankRefresh]`nEnabled=1`nIntervalGameMinutes=4320"; result=9},
    @{name='zero'; ini="[BankRefresh]`nEnabled=1`nIntervalGameMinutes=0"; result=6},
    @{name='too large'; ini="[BankRefresh]`nEnabled=1`nIntervalGameMinutes=4321"; result=6},
    @{name='suffix'; ini="[BankRefresh]`nEnabled=1`nIntervalGameMinutes=15min"; result=6},
    @{name='decimal'; ini="[BankRefresh]`nEnabled=1`nIntervalGameMinutes=15.5"; result=6},
    @{name='negative'; ini="[BankRefresh]`nEnabled=1`nIntervalGameMinutes=-15"; result=6},
    @{name='truncated'; ini="[BankRefresh]`nEnabled=1`nIntervalGameMinutes=$('1'*80)"; result=6},
    @{name='old seconds INI'; ini="[BankRefresh]`nEnabled=1`nIntervalSeconds=60"; result=6},
    @{name='old observe-only INI'; ini="[BankRefresh]`nEnabled=1`nIntervalGameMinutes=15`nDiagnosticOnly=1"; result=7}
)
foreach ($case in $cases) {
    # Generated test fixtures only. The real game and installed INI are never accessed.
    if ($null -ne $case.ini) { [IO.File]::WriteAllText($ini,$case.ini,[Text.Encoding]::ASCII) }
    & (Join-Path $root 'CrimsonDesert.exe') (Join-Path $root 'bank_refresh_instrumented.dll') $case.result
    if ($LASTEXITCODE -ne 0) { throw "Initialization case failed: $($case.name)" }
    if (@(Get-ChildItem -LiteralPath $root -Filter '*.log' -File).Count) { throw 'Unexpected log file.' }
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot '..\Bank_Refresh.ini') -Destination $ini -Force
& (Join-Path $root 'load_tests.exe') (Join-Path $root 'bank_refresh_instrumented.dll') 3
if ($LASTEXITCODE -ne 0) { throw 'Wrong-process guard failed.' }
$before=@(Get-ChildItem -LiteralPath $root -File | Sort-Object Name | ForEach-Object { $_.Name + ':' + (File-Digest $_.FullName) })
foreach ($hostName in @('load_tests.exe','CrimsonDesert.exe')) {
    & (Join-Path $root $hostName) (Join-Path $root 'Bank_Refresh_2.03.02.asi') 0
    if ($LASTEXITCODE -ne 0) { throw 'Production binary load check failed.' }
}
$after=@(Get-ChildItem -LiteralPath $root -File | Sort-Object Name | ForEach-Object { $_.Name + ':' + (File-Digest $_.FullName) })
if (Compare-Object $before $after) { throw 'Production smoke test created or modified files.' }
Write-Output 'PASS: 15 initialization guard cases, 2 production loads; no logs or test-directory writes'
