// SPDX-License-Identifier: GPL-3.0-only

#include <MyALED.h>

constexpr uint8_t kSk6812Pin = 8;
constexpr uint8_t kWs2812Pin = 9;
constexpr uint16_t kLedCount = 14;

MyALED sk6812Leds(kLedCount, kSk6812Pin, LedType::SK6812_WRGB);
MyALED ws2812Leds(kLedCount, kWs2812Pin, LedType::WS2812_GRB);

void setup() {
    // One block per short strip avoids exhausting the shared RMT memory pool.
    const bool sk6812Ready = sk6812Leds.begin(RMT_MEM_NUM_BLOCKS_1);
    const bool ws2812Ready = ws2812Leds.begin(RMT_MEM_NUM_BLOCKS_1);
    if (!sk6812Ready || !ws2812Ready) {
        while (true) {
            delay(1000);
        }
    }
    sk6812Leds.setBrightness(60);
    ws2812Leds.setBrightness(60);

    sk6812Leds.fill(0, 0, kLedCount);
    ws2812Leds.fill(0, 0, kLedCount);
}

void loop() {
    sk6812Leds.setPixelColor(0, 0, 0, 0, 170);
    ws2812Leds.setPixelColor(0, MyALED::Color(170, 0, 0));
    sk6812Leds.show();
    ws2812Leds.show();
    delay(500);

    sk6812Leds.setPixelColor(0, 0, 0, 0, 0);
    ws2812Leds.setPixelColor(0, MyALED::Color(0, 0, 170));
    sk6812Leds.show();
    ws2812Leds.show();
    delay(500);
}
