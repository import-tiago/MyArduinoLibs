Set-StrictMode -Version Latest

function Get-RepositoryRoot {
    return (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
}

function Get-LibraryContext {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    if ($Name -notmatch '^[A-Za-z][A-Za-z0-9_-]*$') {
        throw "Nome de diretorio invalido: '$Name'. Use apenas letras, numeros, '_' e '-'."
    }

    $repositoryRoot = Get-RepositoryRoot
    $libraryRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot $Name))
    if (-not $libraryRoot.StartsWith($repositoryRoot + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "O caminho da biblioteca esta fora do repositorio: $libraryRoot"
    }
    if (-not (Test-Path -LiteralPath $libraryRoot -PathType Container)) {
        throw "Biblioteca nao encontrada: $Name"
    }

    return [pscustomobject]@{
        DirectoryName = $Name
        RepositoryRoot = $repositoryRoot
        LibraryRoot = $libraryRoot
        PackageRoot = Join-Path $libraryRoot 'package'
        DevelopmentRoot = Join-Path $libraryRoot 'development'
        AssetsRoot = Join-Path $libraryRoot 'assets'
    }
}

function Get-LibraryNames {
    $repositoryRoot = Get-RepositoryRoot
    return @(Get-ChildItem -LiteralPath $repositoryRoot -Directory | Where-Object {
        Test-Path -LiteralPath (Join-Path $_.FullName 'package\library.json') -PathType Leaf
    } | Sort-Object Name | Select-Object -ExpandProperty Name)
}

function Read-LibraryProperties {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $properties = @{}
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -match '^\s*([^#;][^=]*)=(.*)$') {
            $properties[$matches[1].Trim()] = $matches[2].Trim()
        }
    }
    return $properties
}

function Test-SemanticVersion {
    param([Parameter(Mandatory = $true)][string]$Version)
    return $Version -match '^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$'
}

function Write-Utf8File {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][AllowEmptyString()][string]$Content
    )

    $parent = Split-Path -Parent $Path
    if ($parent -and -not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Content, $encoding)
}

function Get-PlatformIOExecutable {
    $command = Get-Command pio -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    if ($env:USERPROFILE) {
        $candidate = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\platformio.exe'
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return $candidate
        }
    }

    throw 'PlatformIO nao foi encontrado. Instale-o ou adicione o comando pio ao PATH.'
}

function Get-RequiredCommand {
    param([Parameter(Mandatory = $true)][string]$Name)

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if (-not $command) {
        throw "Comando obrigatorio nao encontrado: $Name"
    }
    return $command.Source
}

function Invoke-CheckedCommand {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$Arguments = @(),
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string]$Description
    )

    Write-Host "  -> $Description"
    Push-Location $WorkingDirectory
    try {
        $previousErrorActionPreference = $ErrorActionPreference
        try {
            $ErrorActionPreference = 'Continue'
            & $FilePath @Arguments 2>&1 | ForEach-Object { Write-Host $_ }
            $exitCode = $LASTEXITCODE
        }
        finally {
            $ErrorActionPreference = $previousErrorActionPreference
        }
        if ($exitCode -ne 0) {
            throw "$Description falhou com codigo de saida $exitCode."
        }
    }
    finally {
        Pop-Location
    }
}

function Get-ManifestAndProperties {
    param([Parameter(Mandatory = $true)]$Context)

    $manifestPath = Join-Path $Context.PackageRoot 'library.json'
    $propertiesPath = Join-Path $Context.PackageRoot 'library.properties'

    try {
        $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    }
    catch {
        throw "library.json nao contem JSON valido: $($_.Exception.Message)"
    }

    return [pscustomobject]@{
        Manifest = $manifest
        Properties = Read-LibraryProperties -Path $propertiesPath
        ManifestPath = $manifestPath
        PropertiesPath = $propertiesPath
    }
}

function Test-MarkdownLinks {
    param([Parameter(Mandatory = $true)]$Context)

    $brokenLinks = New-Object System.Collections.Generic.List[string]
    $documents = Get-ChildItem -LiteralPath $Context.LibraryRoot -Filter '*.md' -File -Recurse | Where-Object {
        $_.FullName -notlike (Join-Path $Context.DevelopmentRoot 'tools\*')
    }

    foreach ($document in $documents) {
        $content = Get-Content -LiteralPath $document.FullName -Raw
        foreach ($match in [regex]::Matches($content, '!?(?:\[[^\]]*\])\(([^)]+)\)')) {
            $reference = $match.Groups[1].Value.Trim()
            if ($reference -match '^(?:https?://|mailto:|#)' -or $reference -match '^<https?://') {
                continue
            }

            $reference = $reference.Trim('<', '>')
            $reference = ($reference -split '#', 2)[0]
            if ([string]::IsNullOrWhiteSpace($reference)) {
                continue
            }

            $target = [System.IO.Path]::GetFullPath((Join-Path $document.DirectoryName $reference))
            if (-not (Test-Path -LiteralPath $target)) {
                $relativeDocument = $document.FullName.Substring($Context.LibraryRoot.Length + 1)
                $brokenLinks.Add("$relativeDocument -> $reference")
            }
        }
    }

    return @($brokenLinks)
}

function Test-LibraryStructure {
    param([Parameter(Mandatory = $true)]$Context)

    $errors = New-Object System.Collections.Generic.List[string]
    $requiredDirectories = @(
        'assets',
        'development',
        'development\src',
        'development\test',
        'package',
        'package\examples',
        'package\src'
    )
    $requiredFiles = @(
        'README.md',
        'development\DEVELOPMENT.md',
        'development\platformio.ini',
        'development\src\main.cpp',
        'package\library.json',
        'package\library.properties',
        'package\keywords.txt'
    )

    foreach ($relativePath in $requiredDirectories) {
        if (-not (Test-Path -LiteralPath (Join-Path $Context.LibraryRoot $relativePath) -PathType Container)) {
            $errors.Add("Diretorio obrigatorio ausente: $relativePath")
        }
    }
    foreach ($relativePath in $requiredFiles) {
        if (-not (Test-Path -LiteralPath (Join-Path $Context.LibraryRoot $relativePath) -PathType Leaf)) {
            $errors.Add("Arquivo obrigatorio ausente: $relativePath")
        }
    }

    foreach ($forbidden in @('.git', '.github')) {
        if (Test-Path -LiteralPath (Join-Path $Context.LibraryRoot $forbidden)) {
            $errors.Add("Conteudo proibido dentro da biblioteca: $forbidden")
        }
    }

    if ($errors.Count -eq 0) {
        $topLevelNames = @(Get-ChildItem -LiteralPath $Context.PackageRoot -Force | Select-Object -ExpandProperty Name)
        $allowedTopLevel = @('examples', 'keywords.txt', 'library.json', 'library.properties', 'src')
        foreach ($name in $topLevelNames) {
            if ($name -notin $allowedTopLevel) {
                $errors.Add("Item nao publicavel encontrado em package/: $name")
            }
        }

        $examples = @(Get-ChildItem -LiteralPath (Join-Path $Context.PackageRoot 'examples') -Filter '*.ino' -File -Recurse)
        if ($examples.Count -eq 0) {
            $errors.Add('package/examples nao contem nenhum arquivo .ino.')
        }
    }

    $platformioPath = Join-Path $Context.DevelopmentRoot 'platformio.ini'
    if (Test-Path -LiteralPath $platformioPath -PathType Leaf) {
        $platformioContent = Get-Content -LiteralPath $platformioPath -Raw
        if ($platformioContent -notmatch '(?m)^\s*symlink://\.\./package\s*$') {
            $errors.Add('development/platformio.ini nao contem a dependencia symlink://../package.')
        }
    }

    if ($errors.Count -gt 0) {
        throw ($errors -join [Environment]::NewLine)
    }
}

function Test-LibraryMetadata {
    param([Parameter(Mandatory = $true)]$Context)

    $data = Get-ManifestAndProperties -Context $Context
    $manifest = $data.Manifest
    $properties = $data.Properties
    $errors = New-Object System.Collections.Generic.List[string]
    $manifestFields = @($manifest.PSObject.Properties.Name)

    foreach ($field in @('name', 'version', 'description', 'authors', 'frameworks', 'platforms', 'headers')) {
        if (-not ($manifestFields -contains $field) -or $null -eq $manifest.$field) {
            $errors.Add("Campo obrigatorio ausente em library.json: $field")
        }
    }
    foreach ($field in @('name', 'version', 'sentence', 'paragraph', 'category', 'url', 'architectures', 'includes')) {
        if (-not $properties.ContainsKey($field) -or [string]::IsNullOrWhiteSpace($properties[$field])) {
            $errors.Add("Campo obrigatorio ausente em library.properties: $field")
        }
    }

    if ($manifestFields -contains 'name' -and $properties.ContainsKey('name') -and $manifest.name -ne $properties['name']) {
        $errors.Add("Nome divergente: library.json='$($manifest.name)', library.properties='$($properties['name'])'.")
    }
    if ($manifestFields -contains 'version' -and $properties.ContainsKey('version') -and $manifest.version -ne $properties['version']) {
        $errors.Add("Versao divergente: library.json='$($manifest.version)', library.properties='$($properties['version'])'.")
    }
    if ($manifestFields -contains 'version' -and $manifest.version -and -not (Test-SemanticVersion -Version ([string]$manifest.version))) {
        $errors.Add("Versao invalida: '$($manifest.version)'. Use MAJOR.MINOR.PATCH.")
    }

    if ($manifestFields -contains 'headers') {
        foreach ($header in @($manifest.headers)) {
            if (-not (Test-Path -LiteralPath (Join-Path $Context.PackageRoot "src\$header") -PathType Leaf)) {
                $errors.Add("Cabecalho declarado em library.json nao encontrado: src/$header")
            }
        }
    }
    if ($properties.ContainsKey('includes')) {
        foreach ($header in $properties['includes'].Split(',')) {
            $trimmedHeader = $header.Trim()
            if ($trimmedHeader -and -not (Test-Path -LiteralPath (Join-Path $Context.PackageRoot "src\$trimmedHeader") -PathType Leaf)) {
                $errors.Add("Cabecalho declarado em library.properties nao encontrado: src/$trimmedHeader")
            }
        }
    }

    $brokenLinks = Test-MarkdownLinks -Context $Context
    foreach ($brokenLink in $brokenLinks) {
        $errors.Add("Link local quebrado: $brokenLink")
    }

    if ($errors.Count -gt 0) {
        throw ($errors -join [Environment]::NewLine)
    }

    return $data
}

function Invoke-LibraryValidation {
    param(
        [Parameter(Mandatory = $true)]$Context,
        [switch]$SkipBuild,
        [switch]$SkipHostTests,
        [switch]$StructureOnly
    )

    Write-Host "Validando $($Context.DirectoryName)..."
    Test-LibraryStructure -Context $Context
    Write-Host '  [OK] Estrutura e conteudo publicavel'

    $data = Test-LibraryMetadata -Context $Context
    Write-Host "  [OK] Manifestos, versao $($data.Manifest.version), cabecalhos e links locais"

    $buildStatus = 'IGNORADO'
    $hostTestStatus = 'IGNORADO'

    if (-not $StructureOnly -and -not $SkipBuild) {
        $pio = Get-PlatformIOExecutable
        Invoke-CheckedCommand -FilePath $pio -Arguments @('run') -WorkingDirectory $Context.DevelopmentRoot -Description 'Compilacao PlatformIO'
        $buildStatus = 'OK'
        Write-Host '  [OK] Compilacao PlatformIO'
    }

    $hostCMake = Join-Path $Context.DevelopmentRoot 'test\host\CMakeLists.txt'
    if (-not $StructureOnly -and -not $SkipHostTests -and (Test-Path -LiteralPath $hostCMake -PathType Leaf)) {
        $cmake = Get-RequiredCommand -Name 'cmake'
        $ctest = Get-RequiredCommand -Name 'ctest'
        $temporaryRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
        $testBuild = [System.IO.Path]::GetFullPath((Join-Path $temporaryRoot "myarduinolibs-$($Context.DirectoryName)-host-tests"))
        if (-not $testBuild.StartsWith($temporaryRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Diretorio temporario inesperado: $testBuild"
        }
        if (Test-Path -LiteralPath $testBuild) {
            Remove-Item -LiteralPath $testBuild -Recurse -Force
        }

        Invoke-CheckedCommand -FilePath $cmake -Arguments @('-S', 'test/host', '-B', $testBuild) -WorkingDirectory $Context.DevelopmentRoot -Description 'Configuracao dos testes nativos'
        Invoke-CheckedCommand -FilePath $cmake -Arguments @('--build', $testBuild) -WorkingDirectory $Context.DevelopmentRoot -Description 'Compilacao dos testes nativos'
        Invoke-CheckedCommand -FilePath $ctest -Arguments @('--test-dir', $testBuild, '--output-on-failure') -WorkingDirectory $Context.DevelopmentRoot -Description 'Execucao dos testes nativos'
        $hostTestStatus = 'OK'
        Write-Host '  [OK] Testes nativos'
    }
    elseif (-not (Test-Path -LiteralPath $hostCMake -PathType Leaf)) {
        $hostTestStatus = 'NAO EXISTEM'
    }

    return [pscustomobject]@{
        Library = $Context.DirectoryName
        Package = [string]$data.Manifest.name
        Version = [string]$data.Manifest.version
        Structure = 'OK'
        Metadata = 'OK'
        Build = $buildStatus
        HostTests = $hostTestStatus
    }
}

function New-ValidatedPackage {
    param(
        [Parameter(Mandatory = $true)]$Context,
        [switch]$KeepPackage
    )

    Invoke-LibraryValidation -Context $Context -StructureOnly | Out-Null
    $pio = Get-PlatformIOExecutable
    $tar = Get-RequiredCommand -Name 'tar'
    $packageValidated = $false

    $existingArchives = @(Get-ChildItem -LiteralPath $Context.PackageRoot -Filter '*.tar.gz' -File -ErrorAction SilentlyContinue)
    if ($existingArchives.Count -gt 0) {
        throw "package/ ja contem arquivo .tar.gz. Remova-o antes de empacotar: $($existingArchives.Name -join ', ')"
    }

    Invoke-CheckedCommand -FilePath $pio -Arguments @('pkg', 'pack') -WorkingDirectory $Context.PackageRoot -Description 'Criacao do pacote PlatformIO'
    $archive = Get-ChildItem -LiteralPath $Context.PackageRoot -Filter '*.tar.gz' -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $archive) {
        throw 'O PlatformIO terminou sem criar um arquivo .tar.gz.'
    }

    try {
        Push-Location $Context.PackageRoot
        try {
            $entries = @(& $tar '-tf' $archive.FullName)
            if ($LASTEXITCODE -ne 0) {
                throw 'Nao foi possivel listar o conteudo do pacote.'
            }
        }
        finally {
            Pop-Location
        }

        $unexpected = @($entries | Where-Object {
            $_ -ne 'library.json' -and
            $_ -ne 'library.properties' -and
            $_ -ne 'keywords.txt' -and
            -not $_.StartsWith('examples/', [System.StringComparison]::Ordinal) -and
            -not $_.StartsWith('src/', [System.StringComparison]::Ordinal)
        })
        if ($unexpected.Count -gt 0) {
            throw "Itens inesperados no pacote: $($unexpected -join ', ')"
        }
        foreach ($requiredEntry in @('library.json', 'library.properties', 'keywords.txt')) {
            if ($requiredEntry -notin $entries) {
                throw "Item obrigatorio ausente no pacote: $requiredEntry"
            }
        }
        if (-not @($entries | Where-Object { $_.StartsWith('examples/', [System.StringComparison]::Ordinal) })) {
            throw 'O pacote nao contem exemplos.'
        }
        if (-not @($entries | Where-Object { $_.StartsWith('src/', [System.StringComparison]::Ordinal) })) {
            throw 'O pacote nao contem codigo-fonte.'
        }

        Write-Host "  [OK] Pacote contem $($entries.Count) arquivos publicaveis"
        $packageValidated = $true
        return [pscustomobject]@{
            Path = $archive.FullName
            Entries = $entries
            Kept = [bool]$KeepPackage
        }
    }
    finally {
        if ((-not $KeepPackage -or -not $packageValidated) -and $archive -and (Test-Path -LiteralPath $archive.FullName)) {
            Remove-Item -LiteralPath $archive.FullName -Force
            Write-Host '  [OK] Pacote temporario removido'
        }
    }
}

Export-ModuleMember -Function @(
    'Get-RepositoryRoot',
    'Get-LibraryContext',
    'Get-LibraryNames',
    'Get-ManifestAndProperties',
    'Get-PlatformIOExecutable',
    'Get-RequiredCommand',
    'Invoke-CheckedCommand',
    'Invoke-LibraryValidation',
    'New-ValidatedPackage',
    'Read-LibraryProperties',
    'Test-SemanticVersion',
    'Write-Utf8File'
)
