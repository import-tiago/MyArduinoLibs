# MyArduinoLibs

[MyArduinoLibs](https://github.com/import-tiago/MyArduinoLibs) organizes the
development of multiple Arduino libraries in a single repository. Each
directory represents an independent library with its own source code, manifest,
version, and package.

The goal is to maintain a standardized workspace without requiring users to
install the entire monorepo. After a library is published to the PlatformIO
Registry, it can be added independently to another project.

## Structure and responsibilities

Each library follows the same organization:

```text
<LIBRARY>/
├── README.md
├── assets/
├── development/
│   ├── DEVELOPMENT.md
│   ├── platformio.ini
│   ├── src/
│   │   └── main.cpp
│   └── test/
└── package/
    ├── library.json
    ├── library.properties
    ├── keywords.txt
    ├── examples/
    └── src/
```

- **`package/`:** content delivered to the user, including the implementation,
  API, manifest, and public examples;
- **`development/`:** complete PlatformIO project used to build, upload, and
  debug directly on the hardware;
- **`assets/`:** datasheets, diagrams, and other internal materials that do not
  need to accompany the installation.

`library.json` is the PlatformIO manifest. `library.properties` and
`keywords.txt` are used when compatibility with tools from the Arduino
ecosystem is also desired. `README.md` and the files under `assets/` document
the library inside the repository, but they are not included with the installed
package because they are outside `package/`.

The library code exists only in `package/src/`. To use it without creating a
copy, `development/platformio.ini` declares a local dependency:

```ini
[env:development]
platform = <platform-id>
board = <board-id>
framework = arduino

lib_deps =
    symlink://../package
```

The `symlink://` protocol makes the development project use the local package
directly and works without manually creating symbolic links in the operating
system.

## The `library.json` manifest

Each `package/` has an independent manifest. A minimal template is:

```json
{
  "$schema": "https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/schema/library.json",
  "name": "LibraryName",
  "version": "0.1.0",
  "description": "Objective description of the library",
  "keywords": ["arduino", "embedded"],
  "repository": {
    "type": "git",
    "url": "https://github.com/OwnerName/RepositoryName.git"
  },
  "authors": [
    {
      "name": "Author Name",
      "maintainer": true
    }
  ],
  "frameworks": ["arduino"],
  "platforms": "*"
}
```

The name, version, and compatibility must represent the published package. When
support is limited, prefer to list only the platforms that have actually been
validated. The optional `export` field controls inclusions and exclusions. See
the [`library.json` format](https://docs.platformio.org/en/latest/manifests/library-json/index.html)
and the [export rules](https://docs.platformio.org/en/latest/manifests/library-json/fields/export/index.html).

## Versioning

Each library evolves independently using Semantic Versioning:

```text
MAJOR.MINOR.PATCH
```

- `MAJOR`: an incompatible API change;
- `MINOR`: new backward-compatible functionality;
- `PATCH`: a backward-compatible fix.

A suitable tag convention for the monorepo is `<LIBRARY>-v<VERSION>`. The tag
version must match `package/library.json`.

## Local development

`development/` must be opened as the PlatformIO project. It contains the
complete environment for building, uploading, and debugging directly on the
hardware, but it does not contain the library implementation.

The driver is edited in `package/src/`. Only the lab firmware that uses this
driver is kept in `development/src/main.cpp`.

### 1. Prepare the environment

Enter the library project:

```powershell
cd <LIBRARY>\development
```

In `platformio.ini`, configure the platform, board, framework, and local package
dependency:

```ini
lib_deps =
    symlink://../package
```

### 2. Build

```powershell
pio run
```

### 3. Upload to the target

```powershell
pio run --target upload
pio debug
```

### 4. Create the publication package

Enter the directory containing `library.json` and run:

```powershell
cd ..\package
pio pkg pack
```

`pio pkg pack` applies the manifest rules and locally creates a file such as
`LibraryName-0.1.0.tar.gz`. It does not build, require a login, or publish
anything. See the
[`pio pkg pack` documentation](https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_pack.html).

### 5. Inspect the publication contents

List the package without extracting it:

```powershell
tar -tf .\LibraryName-0.1.0.tar.gz
```

With the current structure, the archive must contain only:

```text
library.json
library.properties
keywords.txt
examples/
src/
```

It does not contain `README.md`, files from `assets/`, or any other content kept
outside `package/`. The `.tar.gz` is a local verification artifact and normally
should not be versioned.

### 6. Publish

Publishing requires an authenticated account. For manual use:

```powershell
pio account login
pio account show
```

After logging in, publish from `package/`:

```powershell
pio pkg publish --no-interactive
```

`--no-interactive` removes the terminal confirmation, but it does not replace
the login.

Creating a tag or generating the `.tar.gz` does not publish the library.

In addition, a previously published name and version combination cannot be
reused; a correction requires a new version.

See [`pio account login`](https://docs.platformio.org/en/latest/core/userguide/account/cmd_login.html)
and [`pio pkg publish`](https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_publish.html).

## Using it in another project

After publication, the consumer declares only the required package:

```ini
[env:application]
platform = <platform-id>
board = <board-id>
framework = arduino

lib_deps =
    controlandoeletrons/P4RTC @ 0.1.0
```

Specifying the owner, name, and version requirement avoids ambiguity and keeps
the project reproducible. The other directories from the monorepo are not
installed.
