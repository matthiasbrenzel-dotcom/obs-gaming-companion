[CmdletBinding()]
param(
    [ValidateSet('Debug','RelWithDebInfo','Release')]
    [string]$Configuration = 'RelWithDebInfo',
    [string]$ObsDeps = $env:OBS_DEPS_PATH
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

if (-not $ObsDeps) {
    Write-Host 'OBS_DEPS_PATH ist nicht gesetzt.' -ForegroundColor Yellow
    Write-Host 'Setze den Pfad zu einer OBS/libobs CMake-Umgebung oder nutze das offizielle obs-plugintemplate als Build-Umgebung.'
}

$cmakeArgs = @('--preset','windows-x64')
if ($ObsDeps) { $cmakeArgs += "-DCMAKE_PREFIX_PATH=$ObsDeps" }

cmake @cmakeArgs
cmake --build --preset windows-x64 --config $Configuration --parallel
cmake --install build_x64 --config $Configuration --prefix "$root/release/windows-x64"

Write-Host "Build + Install-Staging abgeschlossen: $root/release/windows-x64" -ForegroundColor Green
