[CmdletBinding(SupportsShouldProcess=$true)]
param(
    [string]$ObsPath = '',
    [string]$PackageRoot = "$(Split-Path -Parent $PSScriptRoot)\release\windows-x64"
)
$ErrorActionPreference = 'Stop'

function Find-ObsPath {
    $candidates = @(
        "$env:ProgramFiles\obs-studio",
        "${env:ProgramFiles(x86)}\obs-studio"
    ) | Where-Object { $_ -and (Test-Path (Join-Path $_ 'bin\64bit\obs64.exe')) }
    if ($candidates.Count -gt 0) { return $candidates[0] }
    return ''
}

if (-not $ObsPath) { $ObsPath = Find-ObsPath }
if (-not $ObsPath -or -not (Test-Path (Join-Path $ObsPath 'bin\64bit\obs64.exe'))) {
    throw 'OBS Studio wurde nicht gefunden. Starte mit -ObsPath "C:\Program Files\obs-studio".'
}
if (-not (Test-Path (Join-Path $PackageRoot 'obs-plugins\64bit\obs-gaming-companion.dll'))) {
    throw "Plugin-DLL fehlt in $PackageRoot. Zuerst build-windows.ps1 ausführen."
}

$pluginDst = Join-Path $ObsPath 'obs-plugins\64bit'
$dataDst = Join-Path $ObsPath 'data\obs-plugins\obs-gaming-companion'
if ($PSCmdlet.ShouldProcess($ObsPath, 'OBS Gaming Companion installieren')) {
    New-Item -ItemType Directory -Force $pluginDst | Out-Null
    New-Item -ItemType Directory -Force $dataDst | Out-Null
    Copy-Item (Join-Path $PackageRoot 'obs-plugins\64bit\obs-gaming-companion.dll') $pluginDst -Force
    if (Test-Path (Join-Path $PackageRoot 'data\obs-plugins\obs-gaming-companion')) {
        Copy-Item (Join-Path $PackageRoot 'data\obs-plugins\obs-gaming-companion\*') $dataDst -Recurse -Force
    }
    Write-Host 'OBS Gaming Companion wurde installiert. OBS neu starten.' -ForegroundColor Green
}
