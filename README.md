# MyArduinoLibs

MyArduinoLibs is a monorepo of Arduino libraries developed and validated with
PlatformIO. Each top-level library has its own version, documentation,
development project, and distributable package.

The central rule is simple: **`<LIBRARY>/package/src/` is the only source of
truth for library code.** Development projects compile that package directly;
they never contain synchronized copies of the driver.

## Libraries

| Library | Purpose | Public header | Target |
| --- | --- | --- | --- |
| [LMT01](LMT01/README.md) | Pulse-counting driver for the TI LMT01 temperature sensor | `<LMT01.h>` | ESP32 |
| [LSM6DSM](LSM6DSM/README.md) | I2C driver for the ST LSM6DSM 6-axis IMU | `<LSM6DSM.h>` | ESP32 |
| [P4RTC](P4RTC/README.md) | Epoch-based access to the ESP32-P4 internal RTC and VBAT backup | `<P4RTC.h>` | ESP32-P4 |
| [RV8803](RV8803/README.md) | Epoch-based I2C driver for the Micro Crystal RV-8803-C7 RTC | `<RV8803.h>` | Arduino-compatible boards |

Follow the library link for its API, wiring, examples, compatibility notes, and
datasheets.

## Repository layout

Every library follows this structure:

```text
<LIBRARY>/
├── README.md                 consumer-facing documentation
├── assets/                   datasheets, diagrams and screenshots
├── development/              PlatformIO maintainer workspace
│   ├── DEVELOPMENT.md
│   ├── platformio.ini
│   ├── src/
│   │   └── main.cpp          lab/development firmware
│   └── test/                 PlatformIO tests
└── package/                  independently distributable library
    ├── keywords.txt
    ├── library.json
    ├── library.properties
    ├── examples/
    │   └── BasicUsage/
    │       └── BasicUsage.ino
    └── src/
        ├── <LIBRARY>.h
        └── <LIBRARY>.cpp
```

A development project may contain additional board definitions, scripts, or
tools when they are technically required. Those files stay under
`development/` and are never added to the distributable `package/`.

## Using a library

### From a local checkout

Point PlatformIO at the selected library's `package/` directory. For example,
if this repository is next to your application:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps =
    symlink://../MyArduinoLibs/LMT01/package
```

Then include only its public header:

```cpp
#include <LMT01.h>
```

Adjust the relative path to match your workspace. A symlink dependency means
edits under `package/src/` are used immediately without copying files.

### From the PlatformIO Registry

Once a library is published, install that library independently using its
published owner, name, and version:

```ini
lib_deps =
    <OWNER>/<LIBRARY> @ ^1.0.0
```

The whole monorepo is not a single Arduino library. For Arduino IDE manual
installation, use an archive whose root is the contents of the selected
library's `package/` directory, or use its future release artifact.

## Developing an existing library

Each `development/` directory is a standalone PlatformIO project whose
`platformio.ini` contains:

```ini
lib_deps =
    symlink://../package
```

Build from that directory:

```text
cd <LIBRARY>/development
pio run
```

When tests exist, run:

```text
pio test
```

Use the following locations consistently:

| Content | Location |
| --- | --- |
| Public API and implementation | `package/src/` |
| Public examples | `package/examples/` |
| Development firmware | `development/src/main.cpp` |
| Tests | `development/test/` |
| Maintainer notes | `development/DEVELOPMENT.md` |
| Datasheets and images | `assets/` |

## Adding a library

1. Choose the component or module identifier as the library name when
   practical, such as `ADS1115`.
2. Create the standard directory structure shown above.
3. Put the real driver directly in `package/src/`; do not first create a copy
   under `development/src/`.
4. Add verified metadata to `package/library.json` and
   `package/library.properties`. Do not invent authors, URLs, compatibility, or
   dependencies.
5. Add a minimal public example under
   `package/examples/BasicUsage/BasicUsage.ino`.
6. Create `development/src/main.cpp` for hardware experiments and diagnostics.
7. Configure the development project to consume `symlink://../package`.
8. Document consumer usage in `<LIBRARY>/README.md` and maintainer-specific
   setup in `development/DEVELOPMENT.md`.
9. Store supporting documentation in `assets/`, outside the package.
10. Validate the manifest and run `pio run` before committing.

Libraries may contain multiple headers, implementation files, or
subdirectories under `package/src/` when the design requires them. Avoid
splitting files or exposing implementation details without a technical reason.

## Manifest guidance

Each `package/library.json` should define, as applicable:

- `name` and independent Semantic Versioning `version`;
- an accurate `description` and `keywords`;
- verified `authors` and `repository` metadata;
- `frameworks`, `platforms`, public `headers`, and real dependencies.

Keep `library.properties` consistent with the JSON manifest. `keywords.txt` is
optional for PlatformIO compilation but useful for Arduino IDE syntax
highlighting.

## Versioning and releases

Libraries are versioned independently using `MAJOR.MINOR.PATCH`. A release tag
uses this monorepo convention:

```text
<LIBRARY>-v<VERSION>
```

For example, tag `ADS1115-v1.2.0` must match version `1.2.0` in
`ADS1115/package/library.json`. A release must build the matching
`development/` project and publish only that library's `package/` directory.

## New-library checklist

- [ ] Library README and assets directory exist.
- [ ] `package/src/` contains the only copy of the real library code.
- [ ] `development/src/` contains `main.cpp`, not driver copies.
- [ ] Public examples exist only under `package/examples/`.
- [ ] Manifests exist only under `package/` and contain valid metadata.
- [ ] `development/platformio.ini` uses `symlink://../package`.
- [ ] Public and development code use angle-bracket includes, such as
      `#include <ADS1115.h>`.
- [ ] `pio run` succeeds from `development/`.
- [ ] `pio test` succeeds when tests are present.
- [ ] The release tag and manifest version match.
- [ ] Assets and development-only tools are excluded from `package/`.

The package is the product, `development/` is the laboratory, and this
monorepo is the canonical source of truth.
