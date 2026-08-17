#pragma once

#include <Arduino.h>

class LMT01 {

  public:
    LMT01(uint8_t data_pin, uint8_t power_pin, uint32_t window_ms);
    void begin();
    void handle_pulse();
    bool ready();
    float read();

  private:
    uint8_t _data_pin;
    uint8_t _power_pin;
    uint32_t _window_us;

    hw_timer_t *_timer = nullptr;
    portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
    volatile uint32_t _pulses = 0;
    volatile bool _ready = false;

    void reset();
    static void IRAM_ATTR onTimer();
    static LMT01 *_instance;
};
