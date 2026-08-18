[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Name,

    [switch]$KeepPackage
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'modules\LibraryTools.psm1') -Force

try {
    $context = Get-LibraryContext -Name $Name
    $result = New-ValidatedPackage -Context $context -KeepPackage:$KeepPackage

    Write-Host ''
    Write-Host 'Conteudo validado:'
    $result.Entries | ForEach-Object { Write-Host "  $_" }
    if ($KeepPackage) {
        Write-Host "Pacote preservado em: $($result.Path)"
    }
    else {
        Write-Host 'Pacote validado e removido; use -KeepPackage para preserva-lo.'
    }
}
catch {
    Write-Host "Falha ao empacotar ${Name}: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
