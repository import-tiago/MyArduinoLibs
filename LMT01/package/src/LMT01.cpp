#include "LMT01.h"

LMT01 *LMT01::_instance = nullptr;

LMT01::LMT01(uint8_t data_pin, uint8_t power_pin, uint32_t window_ms) : _data_pin(data_pin), _power_pin(power_pin), _window_us(window_ms * 1000) {
    _instance = this;
}

void LMT01::begin() {
    pinMode(_data_pin, INPUT_PULLUP);
    pinMode(_power_pin, OUTPUT);
    digitalWrite(_power_pin, LOW);

    attachInterrupt(_data_pin, []
    {
        _instance->handle_pulse();
    },
        RISING);

    _timer = timerBegin(1000000); // 1 MHz = 1 µs tick
    timerAttachInterrupt(_timer, &LMT01::onTimer);
    timerAlarm(_timer, _window_us, true, 0); // auto-reload every interval

    digitalWrite(_power_pin, HIGH);
}

void LMT01::handle_pulse() {
    portENTER_CRITICAL_ISR(&_mux);
    if (_pulses == 0)
        timerStart(_timer);
    _pulses += 1;
    portEXIT_CRITICAL_ISR(&_mux);
}

void IRAM_ATTR LMT01::onTimer() {
    digitalWrite(_instance->_power_pin, LOW);
    timerStop(_instance->_timer);

    portENTER_CRITICAL_ISR(&_instance->_mux);
    _instance->_ready = true;
    portEXIT_CRITICAL_ISR(&_instance->_mux);
}

bool LMT01::ready() {
    bool rdy;
    portENTER_CRITICAL(&_mux);
    rdy = _ready;
    portEXIT_CRITICAL(&_mux);
    return rdy;
}

float LMT01::read() {
    float result;
    portENTER_CRITICAL(&_mux);
    result = ((float)_pulses / 4096.0f) * 256.0f - 50.0f;
    portEXIT_CRITICAL(&_mux);
    reset();
    return result;
}

void LMT01::reset() {
    portENTER_CRITICAL(&_mux);
    _ready = false;
    _pulses = 0;
    portEXIT_CRITICAL(&_mux);

    digitalWrite(_power_pin, HIGH);
    timerRestart(_timer);
}
