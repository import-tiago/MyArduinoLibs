# Changelog

All notable changes to MyALED are documented here. Versions follow Semantic Versioning.

## [1.0.0] - 2026-08-03

### Added

- Strongly typed `LedType` protocol and byte-order values.
- Deferred RMT initialization with explicit `begin()` status.
- Configurable RMT block reservation for applications with multiple strips.
- `isValid()` and `isBegun()` state queries.
- Gamma correction, brightness control, dirty-buffer tracking, and dynamic strip length.
- Partial and whole-strip `fill()`, `clear()`, and pixel accessors.
- Failure-aware, transactional buffer allocation using `new (std::nothrow)`.
- Native C++17 unit tests with a simulated Arduino and RMT backend.

### Changed

- Standardized logical colors as `R, G, B, W` and packed colors as `0xWWRRGGBB`.
- Isolated physical RGB/GRB and WRGB/WGRB ordering in the protocol encoders.
- Made `RGBW` parameters const references and query methods const and non-throwing.
- Made packed-color helpers static constexpr functions.
- Made protocol-specific transmission methods private implementation details.
- Split the gamma table into a dedicated translation unit.
- Modernized maintained headers with `#pragma once` and compile-time layout validation.
- Replaced byte-oriented buffer clearing with a type-safe standard algorithm.
- Requires Arduino-ESP32 core 3.3.0 or newer.

### Fixed

- Prevented unsafe shallow copies and moves of objects that own transmission buffers.
- Preserved the active allocation when strip resizing cannot allocate replacement buffers.
- Prevented construction through an unusable partially configured default object.
- Kept logical color representation independent from the selected wire byte order.
