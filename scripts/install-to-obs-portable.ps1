[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$ObsPath,
    [string]$PackageRoot = "$(Split-Path -Parent $PSScriptRoot)\release\windows-x64"
)
& (Join-Path $PSScriptRoot 'install-local.ps1') -ObsPath $ObsPath -PackageRoot $PackageRoot
