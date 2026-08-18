[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Name,

    [switch]$SkipBuild,
    [switch]$SkipHostTests
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'modules\LibraryTools.psm1') -Force

try {
    $context = Get-LibraryContext -Name $Name
    $result = Invoke-LibraryValidation -Context $context -SkipBuild:$SkipBuild -SkipHostTests:$SkipHostTests
    Write-Host ''
    Write-Host "$Name esta valida para desenvolvimento e empacotamento."
    $result | Format-List
}
catch {
    Write-Host "Falha na validacao de ${Name}: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
