[CmdletBinding()]
param(
    [switch]$SkipBuild,
    [switch]$SkipHostTests
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'modules\LibraryTools.psm1') -Force

$names = @(Get-LibraryNames)
if ($names.Count -eq 0) {
    Write-Error 'Nenhuma biblioteca com package/library.json foi encontrada.'
    exit 1
}

$results = New-Object System.Collections.Generic.List[object]
$hasFailure = $false

foreach ($name in $names) {
    Write-Host ''
    try {
        $context = Get-LibraryContext -Name $name
        $result = Invoke-LibraryValidation -Context $context -SkipBuild:$SkipBuild -SkipHostTests:$SkipHostTests
        $results.Add($result)
    }
    catch {
        $hasFailure = $true
        Write-Warning "$name falhou: $($_.Exception.Message)"
        $results.Add([pscustomobject]@{
            Library = $name
            Package = '-'
            Version = '-'
            Structure = 'FALHOU'
            Metadata = 'FALHOU'
            Build = '-'
            HostTests = '-'
        })
    }
}

Write-Host ''
Write-Host 'Resumo:'
$results | Format-Table -AutoSize

if ($hasFailure) {
    exit 1
}
