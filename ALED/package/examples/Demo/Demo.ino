// SPDX-License-Identifier: GPL-3.0-only

#include <MyALED.h>

constexpr uint8_t kLedPin = 8;
constexpr LedType kLedType = LedType::SK6812_WGRB;
constexpr uint16_t kMaximumLedCount = 256;
constexpr uint16_t kLedCount = 12;

MyALED leds(kMaximumLedCount, kLedPin, kLedType);

const uint32_t kRed = MyALED::Color(255, 0, 0);
const uint32_t kBlue = MyALED::Color(0, 0, 255);
const uint32_t kWhite = MyALED::Color(0, 0, 0, 255);

void setup() {
    Serial.begin(115200);
    if (!leds.begin()) {
        while (true) {
            delay(1000);
        }
    }

    // Turn off the maximum allocated range before shrinking to the active size.
    if (!leds.updateLength(kLedCount)) {
        while (true) {
            delay(1000);
        }
    }
    leds.setBrightness(40);
    leds.setGamma(true);
    leds.clear();
    leds.show();
}

void loop() {
    leds.fill(kRed);
    leds.show();
    delay(800);

    RGBW teal = {0, 180, 120, 0};
    leds.fill(teal, 0, leds.numPixels());
    leds.show();
    delay(800);

    leds.clear();
    leds.setPixelColor(0, kBlue);
    leds.setPixelColor(1, 255, 0, 0, 0);
    leds.setPixelColor(2, kWhite);
    leds.show();
    delay(800);

    const uint32_t color = leds.getPixelColor(1);
    Serial.printf("Pixel 1: R=%u G=%u B=%u W=%u\n",
                  MyALED::getRed(color),
                  MyALED::getGreen(color),
                  MyALED::getBlue(color),
                  MyALED::getWhite(color));

    for (uint8_t brightness = 10; brightness <= 120; brightness += 10) {
        leds.setBrightness(brightness);
        leds.show();
        delay(60);
    }
}
