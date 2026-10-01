param(
    [Parameter(Mandatory = $true)][string]$ProxyPath
)

function Read-Exports([string]$Path) {
    $entries = @{}
    $lines = & dumpbin /exports $Path
    if ($LASTEXITCODE -ne 0) { throw "dumpbin failed: $Path" }
    foreach ($line in $lines) {
        if ($line -match '^\s*(\d+)\s+[0-9A-F]+\s+[0-9A-F]+\s+([A-Za-z_][A-Za-z0-9_]*)\s*$') {
            $entries[$Matches[2]] = [int]$Matches[1]
        }
    }
    return $entries
}

$system = Read-Exports "$env:SystemRoot\System32\dxgi.dll"
$proxy = Read-Exports $ProxyPath
$mismatches = @()
foreach ($name in $system.Keys) {
    if (-not $proxy.ContainsKey($name)) {
        $mismatches += "Missing: $name"
        continue
    }
    if ($proxy[$name] -ne $system[$name]) {
        $mismatches += "Ordinal: $name system=$($system[$name]) proxy=$($proxy[$name])"
    }
}
if ($mismatches.Count -gt 0) {
    $mismatches | ForEach-Object { Write-Error $_ }
    exit 1
}
Write-Output "DXGI export names and ordinals match the system DLL"
