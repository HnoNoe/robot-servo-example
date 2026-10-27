$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

$ArmClangBin = if ($env:KEIL_ARMCLANG_BIN) { $env:KEIL_ARMCLANG_BIN } else { 'C:\Keil_v5\ARM\ARMCLANG\bin' }
$env:Path = $ArmClangBin + ';' + $env:Path
New-Item -ItemType Directory -Force -Path build | Out-Null

armclang -c --target=arm-arm-none-eabi -mcpu=cortex-m3 -O2 -std=c99 src/main.c -o build/main.o
if ($LASTEXITCODE -ne 0) { throw 'armclang failed' }
armasm --cpu=Cortex-M3 --apcs=interwork startup/startup_armasm.s -o build/startup.o
if ($LASTEXITCODE -ne 0) { throw 'armasm failed' }
armlink --cpu=Cortex-M3 --scatter=ld/servo.sct --library_type=microlib --entry=Reset_Handler build/main.o build/startup.o -o build/servo_fw.axf
if ($LASTEXITCODE -ne 0) { throw 'armlink failed' }
fromelf --bin --output=build/servo_fw.bin build/servo_fw.axf
fromelf --i32combined --output=build/servo_fw.hex build/servo_fw.axf
Write-Host 'BUILD OK: build\\servo_fw.bin'
