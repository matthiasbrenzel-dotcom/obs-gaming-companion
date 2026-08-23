[CmdletBinding(SupportsShouldProcess=$true)]
param([string]$ObsPath = "$env:ProgramFiles\obs-studio")
$ErrorActionPreference = 'Stop'
if (-not (Test-Path $ObsPath)) { throw "OBS-Pfad nicht gefunden: $ObsPath" }
$dll = Join-Path $ObsPath 'obs-plugins\64bit\obs-gaming-companion.dll'
$data = Join-Path $ObsPath 'data\obs-plugins\obs-gaming-companion'
if ($PSCmdlet.ShouldProcess($ObsPath, 'OBS Gaming Companion deinstallieren')) {
    Remove-Item $dll -Force -ErrorAction SilentlyContinue
    Remove-Item $data -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host 'OBS Gaming Companion wurde entfernt.' -ForegroundColor Green
}
