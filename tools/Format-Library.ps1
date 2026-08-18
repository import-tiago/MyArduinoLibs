[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Name,

    [switch]$Check
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'modules\LibraryTools.psm1') -Force

try {
    $context = Get-LibraryContext -Name $Name

    $clangFormat = $null
    foreach ($commandName in @('clang-format', 'clang-format-18', 'clang-format-17', 'clang-format-16')) {
        $command = Get-Command $commandName -ErrorAction SilentlyContinue
        if ($command) {
            $clangFormat = $command.Source
            break
        }
    }
    if (-not $clangFormat) {
        throw 'clang-format nao foi encontrado no PATH.'
    }

    $libraryStyle = Join-Path $context.DevelopmentRoot '.clang-format'
    $defaultStyle = Join-Path $PSScriptRoot '.clang-format'
    if (Test-Path -LiteralPath $libraryStyle -PathType Leaf) {
        $style = $libraryStyle
    }
    else {
        $style = $defaultStyle
    }

    $searchRoots = @(
        (Join-Path $context.PackageRoot 'src'),
        (Join-Path $context.PackageRoot 'examples'),
        (Join-Path $context.DevelopmentRoot 'src'),
        (Join-Path $context.DevelopmentRoot 'test')
    )
    $extensions = @('.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx', '.ino')
    $files = @($searchRoots | Where-Object { Test-Path -LiteralPath $_ -PathType Container } | ForEach-Object {
        Get-ChildItem -LiteralPath $_ -File -Recurse | Where-Object { $_.Extension -in $extensions }
    } | Sort-Object FullName -Unique)

    if ($files.Count -eq 0) {
        throw 'Nenhum arquivo C/C++ ou .ino foi encontrado.'
    }

    foreach ($file in $files) {
        $arguments = @("--style=file:$style")
        if ($Check) {
            $arguments += @('--dry-run', '--Werror')
        }
        else {
            $arguments += '-i'
        }
        $arguments += $file.FullName

        & $clangFormat @arguments
        if ($LASTEXITCODE -ne 0) {
            throw "Formatacao fora do padrao ou erro em: $($file.FullName)"
        }
    }

    if ($Check) {
        Write-Host "[OK] $($files.Count) arquivos de $Name seguem o padrao de formatacao."
    }
    else {
        Write-Host "[OK] $($files.Count) arquivos de $Name foram formatados. Revise o git diff antes de confirmar as mudancas."
    }
}
catch {
    Write-Host "Falha na formatacao de ${Name}: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
