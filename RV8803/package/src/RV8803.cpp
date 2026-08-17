#include "RV8803.h"

RV8803::RV8803(TwoWire &wirePort) : _wire(wirePort) {
}

time_t RV8803::get_epoch() {
    _wire.beginTransmission(RV8803_ADDR);
    _wire.write(RV8803_SECONDS);
    _wire.endTransmission(false);
    _wire.requestFrom(RV8803_ADDR, (uint8_t)7);

    uint8_t sec = bcdToDec(_wire.read() & 0x7F);
    uint8_t min = bcdToDec(_wire.read() & 0x7F);
    uint8_t hr = bcdToDec(_wire.read() & 0x3F);
    _wire.read();
    uint8_t day = bcdToDec(_wire.read() & 0x3F);
    uint8_t mon = bcdToDec(_wire.read() & 0x1F);
    uint8_t yr = bcdToDec(_wire.read());

    struct tm tm;
    tm.tm_sec = sec;
    tm.tm_min = min;
    tm.tm_hour = hr;
    tm.tm_mday = day;
    tm.tm_mon = mon - 1;
    tm.tm_year = yr + 100;
    tm.tm_isdst = 0;

    return mktime(&tm);
}

bool RV8803::set_epoch(time_t epoch) {
    struct tm *tm = gmtime(&epoch);

    _wire.beginTransmission(RV8803_ADDR);
    _wire.write(RV8803_SECONDS);
    _wire.write(decToBcd(tm->tm_sec));
    _wire.write(decToBcd(tm->tm_min));
    _wire.write(decToBcd(tm->tm_hour));
    _wire.write(weekdayToBit(tm->tm_wday));
    _wire.write(decToBcd(tm->tm_mday));
    _wire.write(decToBcd(tm->tm_mon + 1));
    _wire.write(decToBcd(tm->tm_year % 100));
    return _wire.endTransmission() == 0;
}

uint8_t RV8803::decToBcd(uint8_t val) {
    return (val / 10 * 16) + (val % 10);
}

uint8_t RV8803::bcdToDec(uint8_t val) {
    return (val / 16 * 10) + (val % 16);
}

uint8_t RV8803::weekdayToBit(uint8_t tm_wday) {
    switch (tm_wday) {

        case 0:
            return SUNDAY;
        case 1:
            return MONDAY;
        case 2:
            return TUESDAY;
        case 3:
            return WEDNESDAY;
        case 4:
            return THURSDAY;
        case 5:
            return FRIDAY;
        case 6:
            return SATURDAY;
        default:
            return 0;
    }
}
