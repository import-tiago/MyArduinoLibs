// SPDX-License-Identifier: GPL-3.0-only
// Substantially modified by Tiago Silva on 2026-08-03 from work by Ed Nieuwenhuys.

#pragma once

#include "MyALEDTypes.h"

#include <Arduino.h>

#if !defined(ARDUINO_ARCH_ESP32)
#error "MyALED supports only the ESP32 Arduino core."
#endif

#if !SOC_RMT_SUPPORTED
#error "MyALED requires an ESP32 target with an RMT peripheral."
#endif

class MyALED {
  public:
    MyALED() = delete;
    MyALED(uint16_t ledCount, uint8_t pin, LedType ledType);

    // Compatibility overload for applications that store the LED type as an integer.
    [[deprecated("Use the LedType overload")]] MyALED(uint16_t ledCount, uint8_t pin, uint8_t ledType);

    ~MyALED();

    MyALED(const MyALED &) = delete;
    MyALED &operator=(const MyALED &) = delete;
    MyALED(MyALED &&) = delete;
    MyALED &operator=(MyALED &&) = delete;

    [[nodiscard]] bool begin();
    [[nodiscard]] bool begin(rmt_reserve_memsize_t memoryBlocks);
    [[nodiscard]] bool updateLength(uint16_t ledCount);
    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool isBegun() const noexcept;

    void fill(const RGBW &color, uint16_t firstLed, uint16_t ledCount);
    void fill(uint32_t color, uint16_t firstLed, uint16_t ledCount);
    void fill(uint32_t color);
    void clear();

    void setBrightness(uint8_t brightness);
    [[nodiscard]] uint8_t getBrightness() const noexcept;
    void setGamma(bool enabled);

    void setPixelColor(uint16_t index, const RGBW &color);
    void setPixelColor(uint16_t index, uint32_t color);
    void setPixelColor(uint16_t index, uint8_t red, uint8_t green, uint8_t blue, uint8_t white);
    [[nodiscard]] uint32_t getPixelColor(uint16_t index) const noexcept;

    void show();
    [[nodiscard]] uint16_t numPixels() const noexcept;

    static constexpr uint8_t getWhite(uint32_t color) noexcept {
        return myaled::getWhite(color);
    }

    static constexpr uint8_t getRed(uint32_t color) noexcept {
        return myaled::getRed(color);
    }

    static constexpr uint8_t getGreen(uint32_t color) noexcept {
        return myaled::getGreen(color);
    }

    static constexpr uint8_t getBlue(uint32_t color) noexcept {
        return myaled::getBlue(color);
    }

    static constexpr uint32_t makeRGBWcolor(uint8_t red, uint8_t green, uint8_t blue, uint8_t white) noexcept {
        return myaled::packColor(red, green, blue, white);
    }

    static constexpr uint32_t Color(uint8_t red, uint8_t green, uint8_t blue) noexcept {
        return myaled::packColor(red, green, blue);
    }

    static constexpr uint32_t Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t white) noexcept {
        return myaled::packColor(red, green, blue, white);
    }

  private:
    static bool allocateBuffers(uint16_t ledCount, RGBW *&pixels, rmt_data_t *&ledData);
    void installBuffers(uint16_t ledCount, RGBW *pixels, rmt_data_t *ledData);
    void releaseBuffers();
    void showSK6812();
    void showWS2812();

    uint16_t _ledCount = 0;
    uint8_t _brightness = 128;
    bool _dirty = true;
    bool _useGamma = false;
    bool _begun = false;
    RGBW *_pixels = nullptr;
    rmt_data_t *_ledData = nullptr;
    uint8_t _pin = 0;
    LedType _ledType = LedType::SK6812_WRGB;

    static const uint8_t _gamma8[256];
};
