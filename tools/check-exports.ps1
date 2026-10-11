param(
    [Parameter(Mandatory = $true)][string]$ProxyPath
)

function Read-Exports([string]$Path) {
    $exports = @{ Names = @{}; Ordinals = @{} }
    $lines = & dumpbin /exports $Path
    if ($LASTEXITCODE -ne 0) { throw "dumpbin failed: $Path" }
    foreach ($line in $lines) {
        if ($line -notmatch '^\s*(\d+)\s+(?:[0-9A-Fa-f]+\s+)?[0-9A-Fa-f]{6,8}\s+(.+)$') { continue }
        $ordinal = [int]$Matches[1]
        $exports.Ordinals[$ordinal] = $true
        $entry = $Matches[2]
        if ($entry -match '^([A-Za-z_][A-Za-z0-9_]*)') {
            $exports.Names[$Matches[1]] = $ordinal
        }
    }
    return $exports
}

$system = Read-Exports "$env:SystemRoot\System32\dxgi.dll"
$proxy = Read-Exports $ProxyPath
$missing = @()
foreach ($name in $system.Names.Keys) {
    if (-not $proxy.Names.ContainsKey($name)) { $missing += "Missing name: $name" }
}
foreach ($ordinal in $system.Ordinals.Keys) {
    if (-not $proxy.Ordinals.ContainsKey($ordinal)) { $missing += "Missing ordinal: $ordinal" }
}
if ($missing.Count -gt 0) {
    $missing | ForEach-Object { Write-Error $_ }
    exit 1
}

$smoke = Join-Path (Split-Path -Parent $ProxyPath) "neuralfx_dxgi_smoke.exe"
if (Test-Path $smoke) {
    & $smoke $ProxyPath
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
Write-Output "DXGI export names and ordinal slots are available"
