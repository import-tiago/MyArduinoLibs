[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Name,

    [switch]$DryRun,
    [switch]$Yes,
    [switch]$CreateTag,
    [switch]$KeepPackage
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'modules\LibraryTools.psm1') -Force

$packageResult = $null
try {
    $context = Get-LibraryContext -Name $Name
    $data = Get-ManifestAndProperties -Context $context
    $version = [string]$data.Manifest.version
    $packageName = [string]$data.Manifest.name

    $git = Get-RequiredCommand -Name 'git'
    Push-Location $context.RepositoryRoot
    try {
        $gitStatus = @(& $git 'status' '--porcelain=v1' '--untracked-files=all')
        if ($LASTEXITCODE -ne 0) {
            throw 'Nao foi possivel consultar o estado do Git.'
        }
        if ($gitStatus.Count -gt 0) {
            throw "O repositorio possui mudancas nao confirmadas. Faca commit ou descarte-as antes de publicar.`n$($gitStatus -join [Environment]::NewLine)"
        }
        $branch = (& $git 'branch' '--show-current').Trim()
    }
    finally {
        Pop-Location
    }
    Write-Host "Git limpo; branch atual: $branch"

    $changelogPath = Join-Path $context.AssetsRoot 'CHANGELOG.md'
    if (Test-Path -LiteralPath $changelogPath -PathType Leaf) {
        $changelog = Get-Content -LiteralPath $changelogPath -Raw
        if ($changelog -match "(?m)^## \[$([regex]::Escape($version))\] - Unreleased\s*$") {
            throw "A entrada $version em assets/CHANGELOG.md ainda esta marcada como Unreleased."
        }
    }

    Invoke-LibraryValidation -Context $context | Out-Null
    $packageResult = New-ValidatedPackage -Context $context -KeepPackage
    $archivePath = $packageResult.Path

    if ($DryRun) {
        Write-Host ''
        Write-Host "[DRY RUN] $packageName $version passou pelas verificacoes. Nada foi publicado e nenhuma tag foi criada."
        return
    }

    $pio = Get-PlatformIOExecutable
    Invoke-CheckedCommand -FilePath $pio -Arguments @('account', 'show') -WorkingDirectory $context.PackageRoot -Description 'Verificacao da conta PlatformIO'

    if (-not $Yes) {
        Write-Host ''
        Write-Host "Sera publicado: $packageName $version"
        Write-Host "Origem: $($context.PackageRoot)"
        $confirmation = Read-Host 'Digite PUBLICAR para continuar'
        if ($confirmation -cne 'PUBLICAR') {
            throw 'Publicacao cancelada pelo usuario.'
        }
    }

    Invoke-CheckedCommand -FilePath $pio -Arguments @('pkg', 'publish', '--no-interactive') -WorkingDirectory $context.PackageRoot -Description "Publicacao de $packageName $version"
    Write-Host "[OK] $packageName $version foi publicada no PlatformIO Registry."

    if ($CreateTag) {
        $tag = "$($context.DirectoryName)-v$version"
        Push-Location $context.RepositoryRoot
        try {
            & $git 'rev-parse' '--verify' '--quiet' "refs/tags/$tag" | Out-Null
            if ($LASTEXITCODE -eq 0) {
                throw "A tag ja existe: $tag"
            }
            & $git 'tag' '-a' $tag '-m' "$packageName $version"
            if ($LASTEXITCODE -ne 0) {
                throw "Nao foi possivel criar a tag $tag."
            }
        }
        finally {
            Pop-Location
        }
        Write-Host "[OK] Tag local criada: $tag. Ela ainda nao foi enviada ao GitHub."
    }
}
catch {
    Write-Host "Falha na publicacao de ${Name}: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
finally {
    if ($packageResult -and -not $KeepPackage -and (Test-Path -LiteralPath $packageResult.Path)) {
        Remove-Item -LiteralPath $packageResult.Path -Force
        Write-Host 'Pacote temporario removido.'
    }
}
