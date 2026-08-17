#pragma once

#include <Wire.h>
#include <time.h>

#define RV8803_ADDR 0x32
#define RV8803_SECONDS 0x11
#define RV8803_MINUTES 0x12
#define RV8803_HOURS 0x13
#define RV8803_WEEKDAYS 0x14
#define RV8803_DATE 0x15
#define RV8803_MONTHS 0x16
#define RV8803_YEARS 0x17

#define SUNDAY 0x01
#define MONDAY 0x02
#define TUESDAY 0x04
#define WEDNESDAY 0x08
#define THURSDAY 0x10
#define FRIDAY 0x20
#define SATURDAY 0x40

class RV8803 {

  public:
    RV8803(TwoWire &wirePort = Wire);
    time_t get_epoch();
    bool set_epoch(time_t epoch);

  private:
    TwoWire &_wire;
    uint8_t decToBcd(uint8_t val);
    uint8_t bcdToDec(uint8_t val);
    uint8_t weekdayToBit(uint8_t tm_wday);
};
