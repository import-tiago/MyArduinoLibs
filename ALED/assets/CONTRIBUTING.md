# Contributing

Contributions should keep MyALED focused on ESP32 boards using Arduino-ESP32 3.3.0 or newer.

## Before opening a pull request

1. Keep functional changes separate from formatting or file moves.
2. Format maintained C and C++ files with clang-format 18 and the `development/.clang-format` configuration.
3. Keep examples self-contained. Document any external library dependency in the README.
4. Run Arduino Lint in strict mode.
5. Compile every sketch under `package/examples/` for an ESP32 target.
6. Run the native test suite as documented in `development/DEVELOPMENT.md`.
7. Describe hardware validation when changing RMT timing or byte-order behavior.

Do not add source snapshots for old versions. Update `assets/CHANGELOG.md` when applicable. An `ALED-vX.Y.Z` release tag must match the version in `package/library.properties`.
