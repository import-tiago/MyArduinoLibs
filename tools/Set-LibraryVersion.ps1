[CmdletBinding(SupportsShouldProcess = $true, DefaultParameterSetName = 'ExplicitVersion')]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Name,

    [Parameter(Mandatory = $true, ParameterSetName = 'ExplicitVersion')]
    [ValidatePattern('^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$')]
    [string]$Version,

    [Parameter(Mandatory = $true, ParameterSetName = 'Bump')]
    [ValidateSet('major', 'minor', 'patch')]
    [string]$Bump,

    [switch]$SkipChangelog
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'modules\LibraryTools.psm1') -Force

try {
    $context = Get-LibraryContext -Name $Name
    $data = Get-ManifestAndProperties -Context $context
    $oldVersion = [string]$data.Manifest.version
    if (-not (Test-SemanticVersion -Version $oldVersion)) {
        throw "A versao atual nao segue MAJOR.MINOR.PATCH: $oldVersion"
    }

    $parts = $oldVersion.Split('.') | ForEach-Object { [int]$_ }
    if ($PSCmdlet.ParameterSetName -eq 'Bump') {
        switch ($Bump) {
            'major' { $newVersion = '{0}.0.0' -f ($parts[0] + 1) }
            'minor' { $newVersion = '{0}.{1}.0' -f $parts[0], ($parts[1] + 1) }
            'patch' { $newVersion = '{0}.{1}.{2}' -f $parts[0], $parts[1], ($parts[2] + 1) }
        }
    }
    else {
        $newVersion = $Version
    }

    if ([version]$newVersion -le [version]$oldVersion) {
        throw "A nova versao ($newVersion) deve ser maior que a atual ($oldVersion)."
    }

    if (-not $PSCmdlet.ShouldProcess($Name, "Atualizar versao de $oldVersion para $newVersion")) {
        return
    }

    $manifestContent = Get-Content -LiteralPath $data.ManifestPath -Raw
    $manifestContent = [regex]::Replace(
        $manifestContent,
        '(?m)^(\s*"version"\s*:\s*")[^"]+("\s*,?)$',
        { param($match) $match.Groups[1].Value + $newVersion + $match.Groups[2].Value },
        1
    )
    Write-Utf8File -Path $data.ManifestPath -Content $manifestContent

    $propertiesContent = Get-Content -LiteralPath $data.PropertiesPath -Raw
    $propertiesContent = [regex]::Replace($propertiesContent, '(?m)^version=.*$', "version=$newVersion", 1)
    Write-Utf8File -Path $data.PropertiesPath -Content $propertiesContent

    $readmePath = Join-Path $context.LibraryRoot 'README.md'
    if (Test-Path -LiteralPath $readmePath -PathType Leaf) {
        $readme = Get-Content -LiteralPath $readmePath -Raw
        $packageName = [string]$data.Manifest.name
        $readme = $readme.Replace("$packageName @ $oldVersion", "$packageName @ $newVersion")
        $readme = $readme.Replace("#v$oldVersion", "#v$newVersion")
        $readme = $readme.Replace("Version $oldVersion", "Version $newVersion")
        $readme = $readme.Replace("version $oldVersion", "version $newVersion")
        Write-Utf8File -Path $readmePath -Content $readme
    }

    $changelogPath = Join-Path $context.AssetsRoot 'CHANGELOG.md'
    if (-not $SkipChangelog -and (Test-Path -LiteralPath $changelogPath -PathType Leaf)) {
        $changelog = Get-Content -LiteralPath $changelogPath -Raw
        if ($changelog -notmatch "(?m)^## \[$([regex]::Escape($newVersion))\]") {
            $entry = "## [$newVersion] - Unreleased`r`n`r`n### Changed`r`n`r`n- Describe the changes in this release.`r`n`r`n"
            $firstVersion = [regex]::Match($changelog, '(?m)^## \[')
            if ($firstVersion.Success) {
                $changelog = $changelog.Insert($firstVersion.Index, $entry)
            }
            else {
                $changelog = $changelog.TrimEnd() + "`r`n`r`n" + $entry
            }
            Write-Utf8File -Path $changelogPath -Content $changelog
            Write-Host "Foi criada uma entrada Unreleased em assets/CHANGELOG.md; descreva as mudancas antes de publicar."
        }
    }

    Invoke-LibraryValidation -Context $context -StructureOnly | Out-Null
    Write-Host "[OK] $Name atualizada de $oldVersion para $newVersion."
}
catch {
    Write-Host "Falha ao atualizar a versao de ${Name}: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
