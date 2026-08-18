# MyALED

MyALED is an Arduino library for controlling SK6812 RGBW and WS2812 RGB LEDs with the ESP32 RMT peripheral.

Version 1.0.0 defines a consistent logical color model, makes resource failures observable, and includes automated build and host-test coverage.

## Compatibility

- Arduino-ESP32 core 3.3.0 or newer
- ESP32 targets with an RMT peripheral
- SK6812 in WRGB or WGRB wire order
- WS2812 in RGB or GRB wire order

The local development project targets an ESP32-S3 and uses an Arduino-ESP32 3.3-compatible PlatformIO platform. Actual LED output must still be validated on the target board and strip.

## Installation

### Arduino IDE

Download this repository and copy `ALED/package` to a `MyALED` directory inside the Arduino sketchbook `libraries` directory. Restart the Arduino IDE after copying it.

### PlatformIO

After publication to the PlatformIO Registry, add the package to your project's `platformio.ini`. Pinning the version keeps builds reproducible:

```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps =
    controlandoeletrons/MyALED @ 1.0.0
```

Replace `board` with your ESP32 board identifier and make sure the selected platform supplies Arduino-ESP32 3.3.0 or newer. During development inside this monorepo, open `ALED/development` and use the local dependency already configured there:

```ini
lib_deps =
    symlink://../package
```

### Include

Include the library with:

```cpp
#include <MyALED.h>
```

## Basic usage

```cpp
#include <MyALED.h>

constexpr uint8_t kLedPin = 8;
constexpr uint16_t kLedCount = 24;

MyALED leds(kLedCount, kLedPin, LedType::WS2812_GRB);

void setup() {
    if (!leds.begin()) {
        while (true) {
            delay(1000);
        }
    }

    leds.setBrightness(64);
    leds.fill(MyALED::Color(255, 0, 0));
    leds.show();
}

void loop() {}
```

`begin()` must be called once before the first transmission. RMT initialization is deferred so global objects do not claim peripheral resources before `setup()`.

By default, `begin()` selects an RMT reservation supported by the target. Applications with multiple short strips can reserve fewer shared blocks explicitly:

```cpp
if (!leds.begin(RMT_MEM_NUM_BLOCKS_1)) {
    // Handle RMT allocation failure.
}
```

## Types and color model

Use the scoped `LedType` values:

```cpp
LedType::SK6812_WRGB
LedType::SK6812_WGRB
LedType::WS2812_RGB
LedType::WS2812_GRB
```

The `MyALEDType` alias and unscoped protocol values are available for concise declarations. The constructor accepting a raw `uint8_t` is deprecated because it can represent invalid protocols.

Logical colors always use `R, G, B, W` order:

```cpp
RGBW teal{0, 180, 120, 0};
```

Packed colors use `0xWWRRGGBB`:

```cpp
constexpr uint32_t color = MyALED::Color(0x11, 0x22, 0x33, 0x44);
static_assert(color == 0x44112233);
```

Wire byte order is selected only by `LedType`; it does not change the logical `RGBW` field order or packed-color layout. The white channel is ignored by WS2812 LEDs.

## Resource and error handling

The class uses `nothrow` buffer allocation and exposes its state:

```cpp
bool isValid() const;
bool isBegun() const;
```

Construction can produce an invalid object if its buffers cannot be allocated. Check `isValid()` when the application needs to distinguish allocation failure before calling `begin()`. `begin()` returns `false` for an invalid object or an RMT initialization failure.

`updateLength()` allocates replacement buffers before releasing the current buffers. It returns `false` without replacing the active allocation when memory is unavailable.

A `MyALED` object owns its buffers and one RMT output. It cannot be default-constructed, copied, or moved. Construct one stable instance for each physical output pin.

## Public API

```cpp
MyALED(uint16_t ledCount, uint8_t pin, LedType ledType);

bool begin();
bool begin(rmt_reserve_memsize_t memoryBlocks);
bool updateLength(uint16_t ledCount);
bool isValid() const;
bool isBegun() const;

void fill(const RGBW &color, uint16_t firstLed, uint16_t ledCount);
void fill(uint32_t color, uint16_t firstLed, uint16_t ledCount);
void fill(uint32_t color);
void clear();

void setBrightness(uint8_t brightness);
uint8_t getBrightness() const;
void setGamma(bool enabled);

void setPixelColor(uint16_t index, const RGBW &color);
void setPixelColor(uint16_t index, uint32_t color);
void setPixelColor(uint16_t index, uint8_t red, uint8_t green, uint8_t blue, uint8_t white);
uint32_t getPixelColor(uint16_t index) const;

void show();
uint16_t numPixels() const;
```

The class is not thread-safe. Configure and transmit from one task, or provide external synchronization when sharing an instance.

## Features

- Deferred RMT initialization and explicit initialization status
- Configurable RMT memory reservation for multiple strips
- Target-aware default RMT reservation
- SK6812 reset delay and protocol-specific pulse timings
- Optional gamma correction with `setGamma(true)`
- Dirty-buffer tracking that skips unchanged transmissions
- Zero-initialized, failure-aware pixel and RMT buffers
- Transactional strip resizing with `updateLength()`
- Partial and whole-strip `fill()`, `clear()`, and pixel accessors
- Canonical `RGBW` and `0xWWRRGGBB` color contracts

## Examples

- `Blink`: minimal strip control and initialization failure handling
- `Brightness`: global brightness control
- `Demo`: color, gamma, resizing, fill, clear, and pixel accessors
- `TwoStrips`: independent strips with explicit RMT reservations
- `AdvancedDigitalClock`: configurable 16 x 16 clock using `Time` and `RTClib`

The `AdvancedDigitalClock` example also requires the `Time` and `RTClib` libraries. Low-level Espressif RMT examples are kept under `assets/rmt-reference`. They are reference material, not MyALED API examples, and are not part of the published package.

## Quality checks

Build the hardware development firmware from `ALED/development`:

```powershell
pio run
```

Native C++17 unit tests use a simulated Arduino/RMT backend and can be run from the same directory:

```powershell
cmake -S test/host -B build/host-tests
cmake --build build/host-tests
ctest --test-dir build/host-tests --output-on-failure
```

Host tests cover packed colors, RGBW layout, buffer operations, bounds, brightness, gamma, dirty tracking, resize behavior, RMT initialization failure, lifetime, and protocol byte order. Pulse timing and electrical output still require hardware validation.

## Versioning and releases

Release history is maintained in [assets/CHANGELOG.md](assets/CHANGELOG.md).

Versions follow Semantic Versioning. In this monorepo, use the tag format `ALED-vX.Y.Z`; the version must match both files under `package/`.

## License

MyALED is licensed under [GPL-3.0-only](assets/LICENSE). Third-party reference examples retain their original licenses; see [assets/THIRD_PARTY_NOTICES.md](assets/THIRD_PARTY_NOTICES.md).
