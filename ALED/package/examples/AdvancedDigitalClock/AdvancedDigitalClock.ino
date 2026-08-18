// SPDX-License-Identifier: GPL-3.0-only

// Displays a four-digit clock on a serpentine 16 x 16 WS2812 matrix.
// Additional libraries: Time (TimeLib.h) and RTClib.

#include <MyALED.h>
#include <RTClib.h>
#include <TimeLib.h>
#include <Wire.h>

constexpr uint8_t kLedPin = 8;
constexpr uint16_t kLedCount = 256;
constexpr uint8_t kMatrixWidth = 16;
constexpr uint8_t kMatrixHeight = 16;
constexpr uint32_t kWhite = 0x00FFFFFF;
constexpr bool kTranspose = false;
constexpr bool kFlipX = false;
constexpr bool kFlipY = true;
constexpr bool kSerpentine = true;
constexpr bool kReverseOddRows = true;
constexpr bool kCalibrationMode = false;

MyALED leds(kLedCount, kLedPin, LedType::WS2812_GRB);
RTC_DS3231 rtc;
tmElements_t currentTime;
uint32_t lastMinute;
uint32_t lastSecondTick;

// Digits 0-9, three pixels wide and five pixels high.
const uint8_t PROGMEM kDigits[10][3][5] = {
    {{1, 1, 1, 1, 1}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 1}},
    {{1, 0, 0, 0, 1}, {1, 1, 1, 1, 1}, {0, 0, 0, 0, 1}},
    {{1, 0, 1, 1, 1}, {1, 0, 1, 0, 1}, {1, 1, 1, 0, 1}},
    {{1, 0, 1, 0, 1}, {1, 0, 1, 0, 1}, {1, 1, 1, 1, 1}},
    {{1, 1, 1, 0, 0}, {0, 0, 1, 0, 0}, {1, 1, 1, 1, 1}},
    {{1, 1, 1, 0, 1}, {1, 0, 1, 0, 1}, {1, 0, 1, 1, 1}},
    {{1, 1, 1, 1, 1}, {0, 0, 1, 0, 1}, {0, 0, 1, 1, 1}},
    {{1, 1, 0, 0, 0}, {1, 0, 0, 0, 0}, {1, 1, 1, 1, 1}},
    {{1, 1, 1, 1, 1}, {1, 0, 1, 0, 1}, {1, 1, 1, 1, 1}},
    {{1, 1, 1, 0, 1}, {1, 0, 1, 0, 1}, {1, 1, 1, 1, 1}},
};

void clearLeds();
void updateTime(bool printTime);
void updateEveryMinute();
void placeDigit(uint8_t digit, uint8_t x, uint8_t y);
void placeTime(uint8_t hour, uint8_t minute);
void printRtcTime();
uint16_t mapPixel(uint8_t x, uint8_t y);
void showCalibrationPattern();

void setup() {
    Serial.begin(9600);
    rtc.begin();
    if (!leds.begin()) {
        Serial.println("Failed to initialize MyALED");
        while (true) {
            delay(1000);
        }
    }
    leds.setBrightness(25);
    clearLeds();
    leds.show();
    lastSecondTick = millis();

    if (kCalibrationMode) {
        showCalibrationPattern();
        while (true) {
            delay(1000);
        }
    }
}

void loop() {
    if (millis() - lastSecondTick <= 999) {
        return;
    }

    updateTime(false);
    lastSecondTick = millis();
    if (currentTime.Minute != lastMinute) {
        updateEveryMinute();
    }
}

void updateEveryMinute() {
    lastMinute = currentTime.Minute;
    clearLeds();
    placeTime(currentTime.Hour, currentTime.Minute);
    leds.show();
    printRtcTime();
}

void placeDigit(uint8_t digit, uint8_t x, uint8_t y) {
    for (uint8_t column = 0; column < 3; ++column) {
        for (uint8_t row = 0; row < 5; ++row) {
            if (!pgm_read_byte_near(&kDigits[digit][column][row])) {
                continue;
            }

            leds.setPixelColor(mapPixel(x + column, y + row), kWhite);
        }
    }
}

void placeTime(uint8_t hour, uint8_t minute) {
    placeDigit(hour / 10, 2, 1);
    placeDigit(hour % 10, 7, 1);
    placeDigit(minute / 10, 2, 7);
    placeDigit(minute % 10, 7, 7);
}

void clearLeds() {
    leds.clear();
}

uint16_t mapPixel(uint8_t x, uint8_t y) {
    if (kTranspose) {
        const uint8_t temporary = x;
        x = y;
        y = temporary;
    }
    if (kFlipX) {
        x = kMatrixWidth - 1 - x;
    }
    if (kFlipY) {
        y = kMatrixHeight - 1 - y;
    }

    const bool oddRow = (y % 2) != 0;
    const bool reverse = kSerpentine && (oddRow == kReverseOddRows);
    if (reverse) {
        return y * kMatrixWidth + (kMatrixWidth - 1 - x);
    }
    return y * kMatrixWidth + x;
}

void showCalibrationPattern() {
    leds.clear();
    leds.setPixelColor(mapPixel(0, 0), MyALED::Color(255, 0, 0));
    leds.setPixelColor(mapPixel(1, 0), MyALED::Color(0, 255, 0));
    leds.setPixelColor(mapPixel(0, 1), MyALED::Color(0, 0, 255));
    leds.show();
}

void updateTime(bool printTime) {
    const DateTime rtcTime = rtc.now();
    currentTime.Hour = _min(rtcTime.hour(), 24);
    currentTime.Minute = _min(rtcTime.minute(), 59);
    currentTime.Second = _min(rtcTime.second(), 59);
    currentTime.Day = rtcTime.day();
    currentTime.Month = rtcTime.month();
    currentTime.Year = rtcTime.year() - 2000;
    currentTime.Wday = rtcTime.dayOfTheWeek();

    if (printTime) {
        printRtcTime();
    }
}

void printRtcTime() {
    const DateTime rtcTime = rtc.now();
    char text[40];
    snprintf(text,
             sizeof(text),
             "%02d:%02d:%02d %02d-%02d-%04d",
             rtcTime.hour(),
             rtcTime.minute(),
             rtcTime.second(),
             rtcTime.day(),
             rtcTime.month(),
             rtcTime.year());
    Serial.println(text);
}
