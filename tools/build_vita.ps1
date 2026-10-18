param([string]$SdkPath = $env:VITASDK)
$ErrorActionPreference = 'Stop'
if (-not $SdkPath) { throw 'Pass -SdkPath or set VITASDK to an installed SDK.' }
$env:VITASDK = ([IO.Path]::GetFullPath($SdkPath)).Replace('\','/')
if (-not (Test-Path -LiteralPath "$env:VITASDK/bin/arm-vita-eabi-gcc.exe")) { throw 'ARM compiler not found.' }
$env:PATH = "$env:VITASDK/bin;" + $env:PATH
$taskRoot = Split-Path -Parent $PSScriptRoot
cmake -S "$taskRoot/vita" -B "$taskRoot/build-vita" -G Ninja -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw 'Vita CMake configuration failed.' }
cmake --build "$taskRoot/build-vita" -j2
if ($LASTEXITCODE -ne 0) { throw 'Vita build failed.' }
Write-Output "VPK: $taskRoot/build-vita/sf2-diagnostic.vpk"
