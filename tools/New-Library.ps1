[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidatePattern('^[A-Za-z][A-Za-z0-9_]*$')]
    [string]$Name,

    [ValidatePattern('^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$')]
    [string]$Version = '0.1.0',

    [string]$Description = 'Arduino library created in MyArduinoLibs.',
    [string]$Platform = 'espressif32',
    [string]$Board = 'esp32dev',
    [string]$Architectures = 'esp32',
    [ValidateSet('Display', 'Data Storage', 'Communication', 'Signal Input/Output', 'Sensors', 'Device Control', 'Timing', 'Other')]
    [string]$Category = 'Other',
    [string]$Author = 'Tiago Silva',
    [string]$Email = 'tiagodepaulasilva@gmail.com'
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'modules\LibraryTools.psm1') -Force

try {
    foreach ($value in @($Description, $Platform, $Board, $Architectures, $Category, $Author, $Email)) {
        if ($value -match '[\r\n]') {
            throw 'Os parametros de texto nao podem conter quebras de linha.'
        }
    }

    $repositoryRoot = Get-RepositoryRoot
    $targetRoot = Join-Path $repositoryRoot $Name
    if (Test-Path -LiteralPath $targetRoot) {
        throw "O diretorio ja existe: $targetRoot"
    }

    $templateRoot = Join-Path $PSScriptRoot 'templates\library'
    if (-not (Test-Path -LiteralPath $templateRoot -PathType Container)) {
        throw "Template nao encontrado: $templateRoot"
    }

    if (-not $PSCmdlet.ShouldProcess($targetRoot, 'Criar nova biblioteca')) {
        return
    }

    $descriptionJson = ConvertTo-Json $Description -Compress
    $authorJson = ConvertTo-Json $Author -Compress
    $emailJson = ConvertTo-Json $Email -Compress
    $platformJson = ConvertTo-Json $Platform -Compress

    $tokens = [ordered]@{
        '__LIBRARY_NAME__' = $Name
        '__VERSION__' = $Version
        '__DESCRIPTION__' = $Description
        '__DESCRIPTION_JSON__' = $descriptionJson
        '__PLATFORM__' = $Platform
        '__PLATFORM_JSON__' = $platformJson
        '__BOARD__' = $Board
        '__ARCHITECTURES__' = $Architectures
        '__CATEGORY__' = $Category
        '__AUTHOR__' = $Author
        '__AUTHOR_JSON__' = $authorJson
        '__EMAIL__' = $Email
        '__EMAIL_JSON__' = $emailJson
        '__YEAR__' = [string](Get-Date).Year
    }

    foreach ($template in Get-ChildItem -LiteralPath $templateRoot -File -Recurse -Force) {
        $relativePath = $template.FullName.Substring($templateRoot.Length + 1)
        foreach ($token in $tokens.Keys) {
            $relativePath = $relativePath.Replace($token, $tokens[$token])
        }
        if ($relativePath.EndsWith('.template', [System.StringComparison]::OrdinalIgnoreCase)) {
            $relativePath = $relativePath.Substring(0, $relativePath.Length - '.template'.Length)
        }

        $content = Get-Content -LiteralPath $template.FullName -Raw
        foreach ($token in $tokens.Keys) {
            $content = $content.Replace($token, $tokens[$token])
        }
        Write-Utf8File -Path (Join-Path $targetRoot $relativePath) -Content $content
    }

    $context = Get-LibraryContext -Name $Name
    Invoke-LibraryValidation -Context $context -StructureOnly | Out-Null
    Write-Host "[OK] Biblioteca criada em: $targetRoot"
    Write-Host "Abra $Name\development como projeto PlatformIO e implemente a API em $Name\package\src."
}
catch {
    Write-Host "Falha ao criar ${Name}: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
