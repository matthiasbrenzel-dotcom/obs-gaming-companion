[CmdletBinding()]
param([string]$Iscc = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$iss = Join-Path $root 'installer\obs-gaming-companion.iss'
$dll = Join-Path $root 'release\windows-x64\obs-plugins\64bit\obs-gaming-companion.dll'
if (-not (Test-Path $dll)) { throw 'Build fehlt. Zuerst build-windows.ps1 ausführen.' }
if (-not $Iscc) {
    $candidates = @(
        "$env:ProgramFiles(x86)\Inno Setup 6\ISCC.exe",
        "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
    )
    $Iscc = $candidates | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
}
if (-not $Iscc -or -not (Test-Path $Iscc)) { throw 'Inno Setup 6 / ISCC.exe wurde nicht gefunden.' }
& $Iscc $iss
if ($LASTEXITCODE -ne 0) { throw "ISCC fehlgeschlagen: $LASTEXITCODE" }
Write-Host "Installer erstellt unter: $root\dist" -ForegroundColor Green
