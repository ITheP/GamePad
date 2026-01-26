param(
	[string]$LogPath = ""
)

$defaultAddr2line = Join-Path $env:USERPROFILE ".platformio\packages\toolchain-xtensa-esp32s3\bin\xtensa-esp32s3-elf-addr2line.exe"
$addr2line = $defaultAddr2line

if (-not (Test-Path $addr2line))
{
	throw "addr2line not found: $addr2line"
}

Write-Host "Paste crash log, then press Ctrl+Z and Enter:" -ForegroundColor Cyan

$log = ""
if (-not [string]::IsNullOrWhiteSpace($LogPath))
{
	if (-not (Test-Path $LogPath))
	{
		throw "Log file not found: $LogPath"
	}
	$log = Get-Content -Raw $LogPath
}
else
{
	$log = [Console]::In.ReadToEnd()
	if ([string]::IsNullOrWhiteSpace($log) -and (Test-Path "crash.log"))
	{
		$log = Get-Content -Raw "crash.log"
	}
}

if ([string]::IsNullOrWhiteSpace($log))
{
	throw "No crash log provided. Paste into stdin, or pass -LogPath crash.log"
}

# Extract SHA256 from line like: "ELF file SHA256: d7d94a39a3fdb609"
$shaMatch = [regex]::Match($log, "(?im)^\s*ELF\s+file\s+SHA256\s*:\s*([a-f0-9]{16})\s*$")
if (-not $shaMatch.Success)
{
	throw "No SHA256 found in crash log"
}
$sha = $shaMatch.Groups[1].Value

$artifactDir = Join-Path $PSScriptRoot "artifacts\elf"
if (-not (Test-Path $artifactDir))
{
	throw "Artifacts folder not found: $artifactDir"
}

$elf = Get-ChildItem -Path $artifactDir -Recurse -Filter "*${sha}*.elf" |
	Sort-Object LastWriteTime -Descending |
	Select-Object -First 1

if (-not $elf)
{
	throw "No matching ELF found for SHA256: $sha"
}

$addresses = New-Object System.Collections.Generic.List[string]
function Add-Addr([string]$addr)
{
	if (-not [string]::IsNullOrWhiteSpace($addr) -and -not $addresses.Contains($addr))
	{
		[void]$addresses.Add($addr)
	}
}

# Extract PC values
foreach ($m in [regex]::Matches($log, "(?i)\bpc\s*[:=]\s*(0x[0-9a-fA-F]+)"))
{
	Add-Addr $m.Groups[1].Value
}
foreach ($m in [regex]::Matches($log, "(?i)\bPC\s*:\s*(0x[0-9a-fA-F]+)"))
{
	Add-Addr $m.Groups[1].Value
}

# Extract backtrace addresses
$lines = $log -split "`r?`n"
$btLines = $lines | Where-Object { $_ -match "(?i)\bBacktrace\b" }
foreach ($line in $btLines)
{
	foreach ($m in [regex]::Matches($line, "0x[0-9a-fA-F]+"))
	{
		Add-Addr $m.Value
	}
}

# Fallback: scan all addresses if nothing found
if ($addresses.Count -eq 0)
{
	foreach ($m in [regex]::Matches($log, "0x[0-9a-fA-F]+"))
	{
		Add-Addr $m.Value
	}
}

if ($addresses.Count -eq 0)
{
	throw "No addresses found in crash log"
}

Write-Host "Using ELF: $($elf.FullName)" -ForegroundColor Green
Write-Host "Decoded backtrace for $($addresses.Count) addresses" -ForegroundColor Green

& $addr2line -pfiaC -e $elf.FullName $addresses