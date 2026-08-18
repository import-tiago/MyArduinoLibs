// SPDX-License-Identifier: GPL-3.0-only
// Substantially modified by Tiago Silva on 2026-08-03 from work by Ed Nieuwenhuys.

#include "MyALED.h"

#include <algorithm>
#include <cstddef>
#include <new>

namespace {

#if SOC_RMT_TX_CANDIDATES_PER_GROUP > 2
constexpr rmt_reserve_memsize_t kDefaultRmtMemoryBlocks = RMT_MEM_NUM_BLOCKS_4;
#else
constexpr rmt_reserve_memsize_t kDefaultRmtMemoryBlocks = RMT_MEM_NUM_BLOCKS_2;
#endif

constexpr uint32_t kRmtFrequencyHz = 10000000;
constexpr std::size_t kMaximumSymbolsPerLed = 32;

} // namespace

MyALED::MyALED(uint16_t ledCount, uint8_t pin, LedType ledType) : _pin(pin), _ledType(ledType) {
    if (!myaled::isValidLedType(_ledType)) {
        return;
    }

    RGBW *pixels = nullptr;
    rmt_data_t *ledData = nullptr;
    if (allocateBuffers(ledCount, pixels, ledData)) {
        installBuffers(ledCount, pixels, ledData);
    }
}

MyALED::MyALED(uint16_t ledCount, uint8_t pin, uint8_t ledType)
    : MyALED(ledCount, pin, static_cast<LedType>(ledType)) {}

MyALED::~MyALED() {
    if (_begun) {
        rmtDeinit(_pin);
    }
    releaseBuffers();
}

bool MyALED::allocateBuffers(uint16_t ledCount, RGBW *&pixels, rmt_data_t *&ledData) {
    pixels = nullptr;
    ledData = nullptr;

    if (ledCount == 0) {
        return true;
    }

    pixels = new (std::nothrow) RGBW[ledCount]{};
    if (pixels == nullptr) {
        return false;
    }

    const std::size_t symbolCount = static_cast<std::size_t>(ledCount) * kMaximumSymbolsPerLed + 1U;
    ledData = new (std::nothrow) rmt_data_t[symbolCount]{};
    if (ledData == nullptr) {
        delete[] pixels;
        pixels = nullptr;
        return false;
    }

    return true;
}

void MyALED::installBuffers(uint16_t ledCount, RGBW *pixels, rmt_data_t *ledData) {
    releaseBuffers();
    _pixels = pixels;
    _ledData = ledData;
    _ledCount = ledCount;
    _dirty = true;
}

void MyALED::releaseBuffers() {
    delete[] _pixels;
    delete[] _ledData;
    _pixels = nullptr;
    _ledData = nullptr;
    _ledCount = 0;
}

bool MyALED::begin() {
    return begin(kDefaultRmtMemoryBlocks);
}

bool MyALED::begin(rmt_reserve_memsize_t memoryBlocks) {
    if (_begun) {
        return true;
    }

    if (!isValid()) {
        return false;
    }

    if (!rmtInit(_pin, RMT_TX_MODE, memoryBlocks, kRmtFrequencyHz)) {
        log_e("RMT initialization failed");
        return false;
    }

    _begun = true;
    return true;
}

bool MyALED::updateLength(uint16_t ledCount) {
    if (!myaled::isValidLedType(_ledType)) {
        return false;
    }

    if (ledCount == _ledCount) {
        return ledCount == 0 || isValid();
    }

    RGBW *newPixels = nullptr;
    rmt_data_t *newLedData = nullptr;
    if (!allocateBuffers(ledCount, newPixels, newLedData)) {
        return false;
    }

    if (_begun && _pixels != nullptr && _ledCount > 0) {
        clear();
        show();
    }

    installBuffers(ledCount, newPixels, newLedData);
    return true;
}

bool MyALED::isValid() const noexcept {
    return myaled::isValidLedType(_ledType) && _ledCount > 0 && _pixels != nullptr && _ledData != nullptr;
}

bool MyALED::isBegun() const noexcept {
    return _begun;
}

void MyALED::setBrightness(uint8_t brightness) {
    if (brightness != _brightness) {
        _dirty = true;
    }
    _brightness = brightness;
}

uint8_t MyALED::getBrightness() const noexcept {
    return _brightness;
}

void MyALED::setGamma(bool enabled) {
    if (enabled != _useGamma) {
        _dirty = true;
    }
    _useGamma = enabled;
}

uint16_t MyALED::numPixels() const noexcept {
    return _ledCount;
}

void MyALED::show() {
    if (!_begun || !_dirty || !isValid()) {
        return;
    }

    switch (_ledType) {
        case LedType::SK6812_WRGB:
        case LedType::SK6812_WGRB:
            showSK6812();
            break;

        case LedType::WS2812_RGB:
        case LedType::WS2812_GRB:
            showWS2812();
            break;
    }

    _dirty = false;
}

void MyALED::showSK6812() {
    uint32_t dataBit = 0;

    for (uint32_t index = 0; index < _ledCount; ++index) {
        uint8_t red = _pixels[index].r * _brightness / 255;
        uint8_t green = _pixels[index].g * _brightness / 255;
        uint8_t blue = _pixels[index].b * _brightness / 255;
        uint8_t white = _pixels[index].w * _brightness / 255;

        if (_useGamma) {
            red = _gamma8[red];
            green = _gamma8[green];
            blue = _gamma8[blue];
            white = _gamma8[white];
        }

        uint32_t color = 0;
        if (_ledType == LedType::SK6812_WRGB) {
            color = myaled::packColor(red, green, blue, white);
        } else {
            color = (static_cast<uint32_t>(white) << 24) | (static_cast<uint32_t>(green) << 16) |
                    (static_cast<uint32_t>(red) << 8) | blue;
        }

        for (uint8_t bit = 0; bit < 32; ++bit) {
            if (color & (1UL << (31 - bit))) {
                _ledData[dataBit].level0 = 1;
                _ledData[dataBit].duration0 = 9;
                _ledData[dataBit].level1 = 0;
                _ledData[dataBit].duration1 = 6;
            } else {
                _ledData[dataBit].level0 = 1;
                _ledData[dataBit].duration0 = 3;
                _ledData[dataBit].level1 = 0;
                _ledData[dataBit].duration1 = 12;
            }
            ++dataBit;
        }
    }

    _ledData[dataBit].val = 0;
    rmtWrite(_pin, _ledData, _ledCount * 32, RMT_WAIT_FOR_EVER);
    delayMicroseconds(300);
}

void MyALED::showWS2812() {
    uint32_t dataBit = 0;

    for (uint32_t index = 0; index < _ledCount; ++index) {
        uint8_t red = _pixels[index].r * _brightness / 255;
        uint8_t green = _pixels[index].g * _brightness / 255;
        uint8_t blue = _pixels[index].b * _brightness / 255;

        if (_useGamma) {
            red = _gamma8[red];
            green = _gamma8[green];
            blue = _gamma8[blue];
        }

        uint32_t color = 0;
        if (_ledType == LedType::WS2812_RGB) {
            color = myaled::packColor(red, green, blue);
        } else {
            color = (static_cast<uint32_t>(green) << 16) | (static_cast<uint32_t>(red) << 8) | blue;
        }

        for (uint8_t bit = 0; bit < 24; ++bit) {
            if (color & (1UL << (23 - bit))) {
                _ledData[dataBit].level0 = 1;
                _ledData[dataBit].duration0 = 6;
                _ledData[dataBit].level1 = 0;
                _ledData[dataBit].duration1 = 6;
            } else {
                _ledData[dataBit].level0 = 1;
                _ledData[dataBit].duration0 = 3;
                _ledData[dataBit].level1 = 0;
                _ledData[dataBit].duration1 = 9;
            }
            ++dataBit;
        }
    }

    _ledData[dataBit].val = 0;
    rmtWrite(_pin, _ledData, _ledCount * 24, RMT_WAIT_FOR_EVER);
}

uint32_t MyALED::getPixelColor(uint16_t index) const noexcept {
    if (index >= _ledCount || _pixels == nullptr) {
        return 0;
    }

    const RGBW &color = _pixels[index];
    return myaled::packColor(color.r, color.g, color.b, color.w);
}

void MyALED::setPixelColor(uint16_t index, const RGBW &color) {
    if (index >= _ledCount || _pixels == nullptr) {
        return;
    }

    _pixels[index] = color;
    _dirty = true;
}

void MyALED::setPixelColor(uint16_t index, uint32_t color) {
    setPixelColor(index, myaled::unpackColor(color));
}

void MyALED::setPixelColor(uint16_t index, uint8_t red, uint8_t green, uint8_t blue, uint8_t white) {
    setPixelColor(index, RGBW{red, green, blue, white});
}

void MyALED::fill(const RGBW &color, uint16_t firstLed, uint16_t ledCount) {
    if (_pixels == nullptr || firstLed >= _ledCount) {
        return;
    }

    const uint32_t requestedEnd = static_cast<uint32_t>(firstLed) + ledCount;
    const uint16_t lastLed = requestedEnd > _ledCount ? _ledCount : static_cast<uint16_t>(requestedEnd);

    for (uint16_t index = firstLed; index < lastLed; ++index) {
        _pixels[index] = color;
    }
    _dirty = true;
}

void MyALED::fill(uint32_t color, uint16_t firstLed, uint16_t ledCount) {
    fill(myaled::unpackColor(color), firstLed, ledCount);
}

void MyALED::fill(uint32_t color) {
    fill(color, 0, _ledCount);
}

void MyALED::clear() {
    if (_pixels == nullptr || _ledCount == 0) {
        return;
    }

    std::fill_n(_pixels, _ledCount, RGBW{});
    _dirty = true;
}
