[CmdletBinding()]
param(
    [string]$PackageRoot,
    [string]$ReceiptPath
)
$ErrorActionPreference = 'Stop'
# Use this interpreter's built-in modules even when a parent Vitis/Python shell
# supplies a different PSModulePath (for example, from PowerShell 7).
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility\Microsoft.PowerShell.Utility.psd1') -ErrorAction Stop
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Management\Microsoft.PowerShell.Management.psd1') -ErrorAction Stop
if (-not $PackageRoot) { $PackageRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path) }
$root = [IO.Path]::GetFullPath($PackageRoot).TrimEnd('\','/')
function Resolve-Payload([string]$Relative) {
    if ([IO.Path]::IsPathRooted($Relative) -or $Relative -match '(^|[\\/])\.\.([\\/]|$)' -or $Relative.Contains(':')) { throw "Unsafe relative path: $Relative" }
    $p = [IO.Path]::GetFullPath((Join-Path $root $Relative))
    if (-not $p.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw "Path outside package: $Relative" }
    return $p
}
$expectedCore = @{
    'artifacts/F21C_CANDIDATE_B_R2_TEST_ONLY.bit' = '27ECB5D4CE74494B2220FEFD1FDEA6800F002B711C2834ACF4C2C991441CFFE8'
    'artifacts/F21C_CANDIDATE_B_R2.xsa' = '8383B46D9B801CFBF39DD6AB74B5DAC483E58E8148EAF7F016E594EDBE747B4D'
    'artifacts/F21C_CANDIDATE_B_R2.elf' = 'DF32E985C053DB5FDB7893E06F72479D63BEBEC15C5C6CD55A35BC40A32C278C'
    'init/ps7_init.tcl' = 'F2BEA71C8E6C6D99D60C747101791B93FBD80B4E68F63DE8CB36090D9F780C98'
}
$sumPath = Resolve-Payload 'SHA256SUMS.txt'
$manifestPath = Resolve-Payload 'FILE_MANIFEST.csv'
$seen = @{}
$checked = @()
foreach ($line in Get-Content -LiteralPath $sumPath -Encoding UTF8) {
    if ($line -notmatch '^([0-9A-Fa-f]{64})  (.+)$') { throw "Invalid SHA256SUMS row: $line" }
    $want = $Matches[1].ToUpperInvariant(); $rel = $Matches[2]
    if ($seen.ContainsKey($rel)) { throw "Duplicate checksum path: $rel" }
    $seen[$rel] = $want
    $p = Resolve-Payload $rel
    if (-not (Test-Path -LiteralPath $p -PathType Leaf)) { throw "Missing payload: $rel" }
    $actual = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash
    if ($actual -ne $want) { throw "SHA256 mismatch: $rel" }
    $checked += [pscustomobject]@{ path=$rel; bytes=(Get-Item -LiteralPath $p).Length; sha256=$actual }
}
if (-not $seen.ContainsKey('FILE_MANIFEST.csv')) { throw 'Manifest is not covered by SHA256SUMS' }
$manifest = @(Import-Csv -LiteralPath $manifestPath -Encoding UTF8)
if ($manifest.Count -ne ($checked.Count - 1)) { throw 'Manifest/checksum coverage differs' }
$manifestSeen = @{}
foreach ($row in $manifest) {
    if ($manifestSeen.ContainsKey($row.path)) { throw "Duplicate manifest row: $($row.path)" }
    $manifestSeen[$row.path] = $true
    $p = Resolve-Payload $row.path
    if (-not $seen.ContainsKey($row.path) -or $seen[$row.path] -ne $row.sha256 -or (Get-Item -LiteralPath $p).Length -ne [long]$row.bytes) { throw "Manifest mismatch: $($row.path)" }
}
$outer = @('F21C_CANDIDATE_B_R2_BOARD_TEST_001.zip','ZIP_SHA256.txt','PACKAGE_RECEIPT.json','ZIP_CONTENTS_SHA256.csv')
foreach ($file in Get-ChildItem -LiteralPath $root -Recurse -File -Force) {
    $rel = $file.FullName.Substring($root.Length + 1).Replace('\','/')
    if ($rel -ne 'SHA256SUMS.txt' -and -not $seen.ContainsKey($rel) -and $rel -notin $outer) { throw "Unexpected file; keep logs outside package: $rel" }
}
foreach ($rel in $expectedCore.Keys) {
    if (-not $seen.ContainsKey($rel) -or $seen[$rel] -ne $expectedCore[$rel]) { throw "Not the approved R2 artifact: $rel" }
}
$init = Get-Content -LiteralPath (Resolve-Payload 'init/ps7_init.tcl') -Raw -Encoding UTF8
if ($init -notmatch '(?m)^proc\s+ps7_init\s' -or $init -notmatch '(?m)^proc\s+ps7_post_config\s' -or $init -match '(?m)^proc\s+generate\s') { throw 'Wrong PS initialization Tcl' }
$result = [ordered]@{ task_id='F21C_R2_BOARD_TEST_PACKAGE_001'; status='PASS'; utc=(Get-Date).ToUniversalTime().ToString('o'); package_root=$root; files_verified=$checked.Count + 1; core_artifacts=$expectedCore; files=$checked; board_operation_performed=$false }
if ($ReceiptPath) {
    $out = [IO.Path]::GetFullPath($ReceiptPath)
    if ($out.StartsWith($root + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or $out -eq $root) { throw 'Receipt must be outside immutable package' }
    if (Test-Path -LiteralPath $out) { throw 'Refusing to overwrite receipt' }
    $parent = Split-Path -Parent $out
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) { throw 'Receipt parent directory must exist' }
    [IO.File]::WriteAllText($out,($result | ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false))
}
Write-Output "PACKAGE_VERIFY_PASS files=$($checked.Count + 1) board_operation=NO"
