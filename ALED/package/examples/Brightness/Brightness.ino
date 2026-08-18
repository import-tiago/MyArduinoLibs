// SPDX-License-Identifier: GPL-3.0-only

#include <MyALED.h>

constexpr uint8_t kLedPin = 8;
constexpr uint16_t kLedCount = 24;
constexpr uint8_t kBrightnessStep = 5;

MyALED leds(kLedCount, kLedPin, LedType::WS2812_GRB);

uint8_t brightness = 10;
int16_t brightnessDelta = kBrightnessStep;

void setup() {
    if (!leds.begin()) {
        while (true) {
            delay(1000);
        }
    }
    leds.fill(MyALED::Color(255, 80, 0), 0, kLedCount);
}

void loop() {
    leds.setBrightness(brightness);
    leds.show();
    delay(30);

    const int16_t nextBrightness = static_cast<int16_t>(brightness) + brightnessDelta;
    if (nextBrightness >= 255) {
        brightness = 255;
        brightnessDelta = -kBrightnessStep;
    } else if (nextBrightness <= 0) {
        brightness = 0;
        brightnessDelta = kBrightnessStep;
    } else {
        brightness = static_cast<uint8_t>(nextBrightness);
    }
}
