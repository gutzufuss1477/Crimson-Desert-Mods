param([switch]$Force)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$mod=Join-Path $repo 'mods\bank-refresh'
$release=Join-Path $repo 'release-assets\2.03.02'
$asiName='Bank_Refresh_2.03.02.asi'
$zipName='Bank_Refresh_2.03.02.zip'
$built=Join-Path $mod ('src\out\'+$asiName)
$zip=Join-Path $release $zipName
if (!(Test-Path -LiteralPath $built)) { throw 'Run mods\bank-refresh\src\build.cmd first.' }
if ((Test-Path -LiteralPath $zip) -and !$Force) { throw 'Release ZIP already exists. Use -Force only to rebuild this release intentionally.' }
function Digest([byte[]]$bytes) {
    $algorithm=[Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($algorithm.ComputeHash($bytes))).Replace('-','').ToLowerInvariant() }
    finally { $algorithm.Dispose() }
}
function Utf8-LF([string]$path) {
    return ,([Text.UTF8Encoding]::new($false).GetBytes([IO.File]::ReadAllText($path).Replace("`r`n","`n")))
}
$production=[IO.File]::ReadAllBytes($built)
if ([Text.Encoding]::ASCII.GetString($production).Contains('BankRefresh_TestInitResult')) { throw 'Test-instrumented binary must not be released.' }
$members=[ordered]@{}
$members[$asiName]=$production
$members['Bank_Refresh.ini']=Utf8-LF (Join-Path $mod 'Bank_Refresh.ini')
$members['README.txt']=Utf8-LF (Join-Path $mod 'README.md')
$members['CHANGELOG.txt']=Utf8-LF (Join-Path $mod 'CHANGELOG.txt')
$members['LICENSE.txt']=Utf8-LF (Join-Path $mod 'LICENSE.txt')
$members['MinHook-LICENSE.txt']=Utf8-LF (Join-Path $mod 'src\vendor\minhook\LICENSE.txt')
Copy-Item -LiteralPath $built -Destination (Join-Path $mod $asiName) -Force
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
# Generate a deterministic archive from an explicit allow-list. Never glob saves,
# analysis exports, source build outputs, old test packages or private files.
$zipStream=[IO.File]::Open($zip,[IO.FileMode]::Create,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
$archive=[IO.Compression.ZipArchive]::new($zipStream,[IO.Compression.ZipArchiveMode]::Create,$false)
try {
    foreach ($name in $members.Keys) {
        $entry=$archive.CreateEntry($name,[IO.Compression.CompressionLevel]::Optimal)
        $entry.LastWriteTime=[DateTimeOffset]::new(2026,10,9,0,0,0,[TimeSpan]::Zero)
        $stream=$entry.Open()
        try { $bytes=$members[$name]; $stream.Write($bytes,0,$bytes.Length) }
        finally { $stream.Dispose() }
    }
} finally { $archive.Dispose(); $zipStream.Dispose() }
$archive=[IO.Compression.ZipFile]::OpenRead($zip)
try {
    if ($archive.Entries.Count -ne $members.Count) { throw 'Unexpected archive member count.' }
    foreach ($entry in $archive.Entries) {
        $stream=$entry.Open(); $memory=[IO.MemoryStream]::new()
        try { $stream.CopyTo($memory); if ((Digest $memory.ToArray()) -ne (Digest $members[$entry.FullName])) { throw 'Archive readback mismatch.' } }
        finally { $stream.Dispose(); $memory.Dispose() }
    }
} finally { $archive.Dispose() }
$asiHash=Digest $production
$zipHash=Digest ([IO.File]::ReadAllBytes($zip))
$checksums=Join-Path $release 'SHA256SUMS.txt'
$lines=@([IO.File]::ReadAllLines($checksums) | Where-Object { $_ -notmatch ('\s'+[regex]::Escape($asiName)+'$') -and $_ -notmatch ('\s'+[regex]::Escape($zipName)+'$') })
$lines+=@("$asiHash  $asiName","$zipHash  $zipName")
[IO.File]::WriteAllText($checksums,($lines -join "`n")+"`n",[Text.UTF8Encoding]::new($false))
$sources=[ordered]@{}
$sourceFiles=@(Get-ChildItem -LiteralPath (Join-Path $mod 'src') -Recurse -File | Where-Object {
    $_.FullName -notlike '*\out\*' -and $_.Extension -in @('.cpp','.hpp','.c','.h','.cmd','.ps1','.rc','.json','.txt')
})
$sourceFiles+=@(Get-Item -LiteralPath (Join-Path $mod 'Bank_Refresh.ini'),(Join-Path $mod 'README.md'),(Join-Path $mod 'CHANGELOG.txt'),(Join-Path $mod 'NEXUS_DESCRIPTION.md'),(Join-Path $mod 'LICENSE.txt'))
foreach ($file in $sourceFiles | Sort-Object FullName) {
    $relative=$file.FullName.Substring($repo.Length+1).Replace('\','/')
    $sources[$relative]=Digest (Utf8-LF $file.FullName)
}
$report=[ordered]@{
    mod='Bank Refresh'; mod_version='1.1.0'; game_version='2.03.02'; steam_build='25474236'
    supported_exe_sha256='57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7'
    default_game_minutes=15; default_bond_game_minutes=15; automatic_bond_reinvestment=$false; diagnostic_logging=$false
    asi=@{file=$asiName;sha256=$asiHash;bytes=$production.Length}
    package=@{file=$zipName;sha256=$zipHash;members=@($members.Keys)}
    validation=@{core_checks=24565;host_hook_checks=284;instrumented_guard_cases=33;production_loads=2
        in_game='Gold refresh previously confirmed. User confirmed bond countdown reduced from days to 15 game minutes and accepted manual-start behavior on 2026-10-09. Final release changes only version/name metadata and packaging relative to the accepted bonds test; not a separate final-binary playtest.'}
    source_hash_encoding='UTF-8 without BOM, with LF line endings'; sources=$sources
}
[IO.File]::WriteAllText((Join-Path $repo 'reports\BUILD_REPORT_BANK_REFRESH_1.1.0.json'),($report | ConvertTo-Json -Depth 7)+"`n",[Text.UTF8Encoding]::new($false))
Write-Output "PASS: six allow-listed archive members verified; ASI=$asiHash"
Write-Output $zip
