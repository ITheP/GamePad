# Path to your ELF file
$elfPath = ".\.pio\build\esp32-s3-devkitm-1\firmware.elf"

# Path to addr2line tool
$addr2line = "C:\users\babba\.platformio\packages\toolchain-xtensa-esp32s3\bin\xtensa-esp32s3-elf-addr2line.exe"

# Read crash log from file
$log = Get-Content -Raw "crash.log"

# Extract PC value
$pcMatch = [regex]::Match($log, "PC (0x[0-9a-fA-F]+)")
$pc = $pcMatch.Groups[1].Value

# Extract backtrace addresses (only the PC values before the colon)
$btMatches = [regex]::Matches($log, "(0x[0-9a-fA-F]+):0x[0-9a-fA-F]+")
$backtrace = $btMatches | ForEach-Object { $_.Groups[1].Value }

# Combine PC + backtrace addresses
$addresses = @($pc) + $backtrace

Write-Host "Decoded backtrace for addresses: $($addresses -join ', ')"

# Run addr2line with all addresses
& $addr2line -pfia -e $elfPath $addresses