#include <Arduino.h>
#include <MyALED.h>

constexpr uint8_t kLedPin = 8;
constexpr uint16_t kLedCount = 24;

MyALED leds(kLedCount, kLedPin, LedType::WS2812_GRB);

void setup() {
    Serial.begin(115200);

    if (!leds.begin()) {
        Serial.println("Unable to initialize the LED strip.");
        while (true) {
            delay(1000);
        }
    }

    leds.setBrightness(64);
    leds.clear();
    leds.show();
}

void loop() {
    leds.setPixelColor(0, MyALED::Color(255, 0, 0));
    leds.show();
    delay(500);

    leds.setPixelColor(0, MyALED::Color(0, 0, 255));
    leds.show();
    delay(500);
}
