$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Bin = Join-Path $Root 'build\servo_fw.bin'
if (-not (Test-Path $Bin)) { throw "Missing $Bin, run build.ps1 first" }

$port = 'COM7'
if ($args.Count -ge 1) { $port = $args[0] }

Write-Host "=== ISP Flash ==="
Write-Host "1) Set BOOT0=1, BOOT1=0"
Write-Host "2) Press RESET"
Write-Host "3) Keep USB on USB_232 ($port)"
Write-Host "Flashing $Bin to $port ..."

python -m stm32loader -p $port -b 115200 -e -w -v $Bin
if ($LASTEXITCODE -ne 0) { throw "Flash failed ($LASTEXITCODE)" }

Write-Host "Flash OK. Now set BOOT0=0, BOOT1=0, press RESET."
