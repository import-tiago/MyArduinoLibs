# Local library automation

This directory contains local PowerShell automation for creating, checking,
versioning, packaging, formatting, and publishing the libraries in
MyArduinoLibs. The scripts do not use a continuous integration service.

Run the commands from the repository root with Windows PowerShell or
PowerShell 7:

```powershell
.\tools\Test-Library.ps1 -Name ALED
```

If Windows blocks unsigned local scripts, run the command only for the current
process without changing the machine policy:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\Test-Library.ps1 -Name ALED
```

## Requirements

- PowerShell 5.1 or newer;
- PlatformIO available as `pio`, or installed in the standard user directory;
- Git and `tar` for publication and package inspection;
- CMake and CTest only for libraries containing native tests;
- clang-format only when using `Format-Library.ps1`.

The shared implementation is in `modules/LibraryTools.psm1`. Templates used by
`New-Library.ps1` are under `templates/library`.

## Test-Library.ps1

Checks whether one library is structurally consistent and whether its code can
be built and tested:

```powershell
.\tools\Test-Library.ps1 -Name ALED
```

It checks all of the following:

- required directories: `assets`, `development`, and `package`;
- required development files, including `platformio.ini` and `src/main.cpp`;
- required package files, including both manifests and `keywords.txt`;
- absence of a nested `.git` or `.github` directory;
- that `package/` contains only `src`, `examples`, and the three metadata files;
- that at least one `.ino` example exists;
- that `development/platformio.ini` contains `symlink://../package`;
- that `library.json` is valid JSON and contains its required fields;
- that `library.json` and `library.properties` contain the same package name and
  version;
- that the version uses the `MAJOR.MINOR.PATCH` format;
- that every declared public header exists under `package/src`;
- that local links in the library Markdown documents point to existing files;
- that the PlatformIO development firmware compiles;
- that native CMake tests compile and pass when
  `development/test/host/CMakeLists.txt` exists.

To check only files and metadata, without compiling firmware:

```powershell
.\tools\Test-Library.ps1 -Name ALED -SkipBuild -SkipHostTests
```

`-SkipBuild` skips the PlatformIO compilation. `-SkipHostTests` skips native
CMake tests but does not skip the firmware compilation.

## Pack-Library.ps1

Creates the same `.tar.gz` package that would be submitted to the PlatformIO
Registry and inspects every entry in the archive:

```powershell
.\tools\Pack-Library.ps1 -Name ALED
```

Before packaging, it checks the directory structure and manifests. After
running `pio pkg pack`, it rejects the archive if it contains anything outside:

```text
library.json
library.properties
keywords.txt
examples/
src/
```

By default, the archive is deleted after inspection because it is only a local
verification artifact. Preserve it when needed with:

```powershell
.\tools\Pack-Library.ps1 -Name ALED -KeepPackage
```

## New-Library.ps1

Creates a new library from the standard template:

```powershell
.\tools\New-Library.ps1 `
    -Name BMP280 `
    -Description "Arduino driver for the BMP280 pressure sensor." `
    -Category Sensors
```

It creates:

- the `assets`, `development`, and `package` directories;
- a development PlatformIO project linked with `symlink://../package`;
- `library.json`, `library.properties`, and `keywords.txt`;
- initial `.h` and `.cpp` source files;
- a `BasicUsage.ino` example;
- initial library and development documentation.

Useful optional parameters include:

```powershell
-Version 0.1.0
-Platform espressif32
-Board esp32dev
-Architectures esp32
-Author "Tiago Silva"
-Email "tiagodepaulasilva@gmail.com"
```

The library name must also be a valid C++ identifier because it becomes the
initial class and header name. The script refuses to overwrite an existing
directory. Use the standard PowerShell `-WhatIf` parameter to preview the
target without creating it:

```powershell
.\tools\New-Library.ps1 -Name BMP280 -WhatIf
```

## Set-LibraryVersion.ps1

Changes a library version without allowing a downgrade:

```powershell
.\tools\Set-LibraryVersion.ps1 -Name ALED -Version 1.1.0
```

It updates:

- `package/library.json`;
- `package/library.properties`;
- exact package-version references in the library README;
- `assets/CHANGELOG.md`, when present.

When updating the changelog, it creates an `Unreleased` entry that must be
completed before publication. Suppress that entry with `-SkipChangelog`.

The version can also be calculated from the current one:

```powershell
.\tools\Set-LibraryVersion.ps1 -Name ALED -Bump patch
.\tools\Set-LibraryVersion.ps1 -Name ALED -Bump minor
.\tools\Set-LibraryVersion.ps1 -Name ALED -Bump major
```

Preview the affected version without editing files:

```powershell
.\tools\Set-LibraryVersion.ps1 -Name ALED -Bump patch -WhatIf
```

## Test-AllLibraries.ps1

Discovers every directory containing `package/library.json`, runs the same
checks as `Test-Library.ps1`, and prints a summary table:

```powershell
.\tools\Test-AllLibraries.ps1
```

The table reports separately whether structure, metadata, firmware compilation,
and native tests passed. One failing library does not prevent the remaining
libraries from being checked, but the command returns a failure exit code at
the end.

For a fast repository-wide metadata check:

```powershell
.\tools\Test-AllLibraries.ps1 -SkipBuild -SkipHostTests
```

## Publish-Library.ps1

Performs the complete pre-publication workflow and then publishes one package
to the PlatformIO Registry:

```powershell
.\tools\Publish-Library.ps1 -Name ALED
```

Before publishing, it verifies:

- that the Git working tree has no tracked or untracked changes;
- that the current changelog entry is not marked `Unreleased`;
- all structure and metadata rules checked by `Test-Library.ps1`;
- successful PlatformIO compilation;
- successful native tests, when present;
- that the generated package contains only public files;
- that a PlatformIO account is authenticated.

The script displays the package name and version and requires the exact word
`PUBLICAR` before calling `pio pkg publish --no-interactive`.

Always start with a simulation:

```powershell
.\tools\Publish-Library.ps1 -Name ALED -DryRun
```

`-DryRun` performs compilation, tests, and packaging but does not publish or
create a tag. Other optional parameters are:

- `-Yes`: skips the typed confirmation, suitable only for an intentional local
  automation;
- `-CreateTag`: creates a local annotated tag such as `ALED-v1.0.0` after a
  successful publication;
- `-KeepPackage`: preserves the generated `.tar.gz`.

The script never commits changes, pushes commits, or pushes tags.

## Format-Library.ps1

Uses clang-format to make C, C++, header, and `.ino` files follow one visual
style. It checks files under `package/src`, `package/examples`,
`development/src`, and `development/test`.

To report formatting differences without modifying files:

```powershell
.\tools\Format-Library.ps1 -Name ALED -Check
```

To correct indentation, spaces, line breaks, include ordering, and brace
placement automatically:

```powershell
.\tools\Format-Library.ps1 -Name ALED
```

The script uses `development/.clang-format` when the library provides one;
otherwise, it uses `tools/.clang-format`. Formatting does not compile the code,
execute tests, change versions, package, or publish anything. Always inspect
`git diff` after applying automatic formatting.

## Recommended workflows

### Create a library

```powershell
.\tools\New-Library.ps1 -Name BMP280 -Description "Arduino BMP280 driver." -Category Sensors
cd .\BMP280\development
pio run
```

### Validate an existing change

```powershell
.\tools\Format-Library.ps1 -Name ALED -Check
.\tools\Test-Library.ps1 -Name ALED
.\tools\Pack-Library.ps1 -Name ALED
```

### Prepare and publish a release

```powershell
.\tools\Set-LibraryVersion.ps1 -Name ALED -Bump patch
# Complete assets/CHANGELOG.md and commit the changes.
.\tools\Publish-Library.ps1 -Name ALED -DryRun
.\tools\Publish-Library.ps1 -Name ALED -CreateTag
```
