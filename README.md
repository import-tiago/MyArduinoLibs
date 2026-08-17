# MyArduinoLibs

MyArduinoLibs is a monorepo for developing, validating, and independently
distributing Arduino libraries with PlatformIO.

Each top-level directory represents an independent library with its own source
code, manifest, version, documentation, development project, and distributable
package. This keeps development standardized without requiring users to install
the entire monorepo.

## Libraries

| Library | Purpose | Public header | Target |
| --- | --- | --- | --- |
| [LMT01](LMT01/README.md) | Pulse-counting driver for the TI LMT01 temperature sensor | `<LMT01.h>` | ESP32 |
| [LSM6DSM](LSM6DSM/README.md) | I2C driver for the ST LSM6DSM 6-axis IMU | `<LSM6DSM.h>` | ESP32 |
| [P4RTC](P4RTC/README.md) | Epoch-based access to the ESP32-P4 internal RTC and VBAT backup | `<P4RTC.h>` | ESP32-P4 |
| [RV8803](RV8803/README.md) | Epoch-based I2C driver for the Micro Crystal RV-8803-C7 RTC | `<RV8803.h>` | Arduino-compatible boards |

Follow a library link for its API, wiring, examples, compatibility notes, and
datasheets.

## Structure and responsibilities

Each library follows the same base organization:

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
    │   └── BasicUsage/
    │       └── BasicUsage.ino
    └── src/
        ├── <LIBRARY>.h
        └── <LIBRARY>.cpp
```

- **`package/`** is the distributable library. It contains the implementation,
  public API, manifests, and public examples.
- **`development/`** is a complete PlatformIO project used to compile and test
  the package and, when hardware support is configured, upload and debug the
  development firmware.
- **`assets/`** contains datasheets, diagrams, screenshots, and other supporting
  material that should not be installed with the library.
- **`<LIBRARY>/README.md`** is the canonical consumer documentation inside the
  monorepo. It is intentionally not duplicated manually under `package/`.

Some development projects may require extra board definitions, scripts, or
tools. P4RTC, for example, keeps its custom ESP32-P4 board and USB-JTAG support
under `development/`. Development-only files must never be copied into
`package/`.

`library.json` is the PlatformIO manifest. `library.properties` and
`keywords.txt` provide compatibility and metadata for the broader Arduino
ecosystem.

Files outside `package/` are not included when the package is packed or
published. If a future release must include a README, license, or image, that
file must be added to the package by the release process or use a public URL.

## Single source of truth

The real library code exists only in:

```text
<LIBRARY>/package/src/
```

The development project must not contain synchronized copies of public headers
or implementation files. Instead, `development/platformio.ini` declares the
local package as a dependency:

```ini
lib_deps =
    symlink://../package
```

The `symlink://` protocol makes PlatformIO consume the local package directly.
No manual operating-system symlink and no copy script are required. Changes in
`package/src/` are therefore used by the next development build immediately.

## The `library.json` manifest

Each `package/` directory has an independent manifest. A minimal template is:

```json
{
  "$schema": "https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/schema/library.json",
  "name": "LibraryName",
  "version": "0.1.0",
  "description": "A concise description of the library",
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
  "frameworks": "arduino",
  "platforms": "*",
  "headers": "LibraryName.h"
}
```

The name, version, authors, repository, compatibility, headers, and dependencies
must describe the package being published. When support is limited, list only
platforms that are intentionally supported and validated.

The optional `license` field accepts an SPDX expression, such as `MIT` or
`Apache-2.0`. Add it only after the project license has been selected and the
corresponding license text is available; do not use a placeholder as a real
license value.

The optional `export` field controls which files are included or excluded.
Because `package/` is already the publication boundary in this repository, it
normally does not need to exclude monorepo development files.

References:

- [PlatformIO `library.json` format](https://docs.platformio.org/en/latest/manifests/library-json/index.html)
- [PlatformIO export rules](https://docs.platformio.org/en/latest/manifests/library-json/fields/export/index.html)

## Versioning

Each library evolves independently using Semantic Versioning:

```text
MAJOR.MINOR.PATCH
```

- `MAJOR`: an incompatible public API change;
- `MINOR`: backward-compatible functionality;
- `PATCH`: a backward-compatible fix.

The monorepo tag convention is:

```text
<LIBRARY>-v<VERSION>
```

The tag version must match `package/library.json`. For example,
`P4RTC-v0.1.0` must point to a commit where `P4RTC/package/library.json`
contains version `0.1.0`.

## Local development

The `development/` directory is the PlatformIO workspace, but the driver itself
must still be edited under `package/src/`.

### 1. Open the development project

From the repository root:

```powershell
cd <LIBRARY>\development
```

Use these locations while working:

```text
../package/src/     library implementation and public API
src/main.cpp        lab firmware and hardware experiments
test/               PlatformIO tests
DEVELOPMENT.md      board setup and maintainer notes
```

The existing `platformio.ini` already selects the supported platform, board,
framework, and local package dependency. Do not replace library includes with
relative paths such as `../../package/src/LibraryName.h`.

### 2. Build

```powershell
pio run
```

The build must resolve the public header through the PlatformIO Library
Dependency Finder. Application code should use an angle-bracket include:

```cpp
#include <LibraryName.h>
```

### 3. Upload or debug when supported

With a compatible board connected and the upload method correctly configured:

```powershell
pio run --target upload
```

Debugging is optional and requires a supported debug interface and matching
PlatformIO configuration:

```powershell
pio debug
```

These commands are hardware-specific. A successful build does not by itself
guarantee that upload or debugging is configured for every board.

### 4. Run tests when available

```powershell
pio test
```

Hardware-dependent tests may require a connected target or a dedicated test
environment.

### 5. Create a publication archive

Enter the directory that contains `library.json` and pack it:

```powershell
cd ..\package
pio pkg pack
```

`pio pkg pack` validates the manifest, applies packaging rules, and creates a
local archive such as `LibraryName-0.1.0.tar.gz`. It does not compile the
library, require an authenticated account, or publish anything.

See the [`pio pkg pack` documentation](https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_pack.html).

### 6. Inspect the archive

List its contents without extracting it:

```powershell
tar -tf .\LibraryName-0.1.0.tar.gz
```

For the current repository architecture, the archive should contain only the
contents of `package/`, typically:

```text
library.json
library.properties
keywords.txt
examples/
src/
```

It must not contain `development/`, `assets/`, build artifacts, or another
library. Generated `.tar.gz` archives are local verification artifacts and
should not be committed.

### 7. Publish manually

Publishing requires an authenticated PlatformIO account:

```powershell
pio account login
pio account show
```

After logging in, publish from the selected library's `package/` directory:

```powershell
pio pkg publish --no-interactive
```

`--no-interactive` removes the terminal confirmation; it does not replace
authentication. Creating a Git tag or a `.tar.gz` archive does not publish the
library.

A package name and version combination cannot be reused after publication. Any
fix requires a new version in `library.json` before republishing.

See the [`pio pkg publish` documentation](https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_publish.html).

## Using a library in another project

### Install from the PlatformIO Registry

After publication, a consumer declares only the required package. P4RTC is
currently available as version `0.1.0`:

```ini
[env:esp32-p4]
platform = espressif32
board = <esp32-p4-board-id>
framework = arduino

lib_deps =
    controlandoeletrons/P4RTC @ 0.1.0
```

Using an exact version makes the dependency reproducible. A compatible version
range such as `^0.1.0` may be used when automatic compatible updates are
desired.

The other libraries can use the same owner/name/version form after they have
been published independently.

### Use a local checkout

Before publication, or while developing an application and a library together,
point the consumer project directly at the selected package:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps =
    symlink://../MyArduinoLibs/LMT01/package
```

Adjust the relative path for the actual workspace layout.

### Include the public API

User firmware includes only the public header and does not need to know about
the monorepo layout. For example:

```cpp
#include <P4RTC.h>

P4RTC rtc;

void setup() {
    rtc.enable_vbat_backup();
}

void loop() {
    time_t now = rtc.get_epoch();
    // Use the current epoch value.
}
```

Always consult the selected library's README for supported hardware,
initialization, wiring, and API details. Installing a Registry package does not
install the other libraries, `development/`, or `assets/` from this monorepo.

## Final principle

```text
package/src/          real library implementation and single source of truth
development/src/      development application only
development/test/     tests
package/examples/     public user examples
assets/               datasheets and supporting documentation
```

The package is the product, `development/` is the laboratory, and the
MyArduinoLibs monorepo is the canonical source of truth.
