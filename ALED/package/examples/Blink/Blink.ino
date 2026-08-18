// SPDX-License-Identifier: GPL-3.0-only

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
    leds.fill(0, 0, kLedCount);
}

void loop() {
    leds.setPixelColor(0, MyALED::Color(255, 0, 0));
    leds.show();
    delay(500);

    leds.setPixelColor(0, MyALED::Color(0, 0, 255));
    leds.show();
    delay(500);
}
