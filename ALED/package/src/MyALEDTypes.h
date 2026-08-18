// SPDX-License-Identifier: GPL-3.0-only
// Substantially modified by Tiago Silva on 2026-08-03 from work by Ed Nieuwenhuys.

#pragma once

#include <stdint.h>

enum class LedType : uint8_t {
    SK6812_WRGB = 10,
    SK6812_WGRB = 11,
    WS2812_RGB = 20,
    WS2812_GRB = 21,
};

// Concise aliases for applications that prefer the original protocol spellings.
using MyALEDType = LedType;
inline constexpr LedType SK6812WRGB = LedType::SK6812_WRGB;
inline constexpr LedType SK6812WGRB = LedType::SK6812_WGRB;
inline constexpr LedType WS2812RGB = LedType::WS2812_RGB;
inline constexpr LedType WS2812GRB = LedType::WS2812_GRB;

// Logical color order is always R, G, B, W. Wire byte order is selected by LedType.
struct RGBW {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t w;
};

static_assert(sizeof(RGBW) == 4, "RGBW must remain a four-byte value type");

namespace myaled {

constexpr uint32_t packColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t white = 0) noexcept {
    return (static_cast<uint32_t>(white) << 24) | (static_cast<uint32_t>(red) << 16) |
           (static_cast<uint32_t>(green) << 8) | blue;
}

constexpr uint8_t getWhite(uint32_t color) noexcept {
    return static_cast<uint8_t>(color >> 24);
}

constexpr uint8_t getRed(uint32_t color) noexcept {
    return static_cast<uint8_t>(color >> 16);
}

constexpr uint8_t getGreen(uint32_t color) noexcept {
    return static_cast<uint8_t>(color >> 8);
}

constexpr uint8_t getBlue(uint32_t color) noexcept {
    return static_cast<uint8_t>(color);
}

constexpr RGBW unpackColor(uint32_t color) noexcept {
    return {getRed(color), getGreen(color), getBlue(color), getWhite(color)};
}

constexpr bool isValidLedType(LedType type) noexcept {
    return type == LedType::SK6812_WRGB || type == LedType::SK6812_WGRB || type == LedType::WS2812_RGB ||
           type == LedType::WS2812_GRB;
}

} // namespace myaled
