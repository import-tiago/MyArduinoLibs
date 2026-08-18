// SPDX-License-Identifier: GPL-3.0-only

#include "FakeArduino.h"

namespace {

fake_arduino::State gState;

} // namespace

namespace fake_arduino {

State &state() {
    return gState;
}

void reset() {
    gState = {};
}

std::vector<uint8_t> decodeLastWriteBytes() {
    std::vector<uint8_t> bytes;
    if (gState.lastWrite.size() % 8 != 0) {
        return bytes;
    }

    bytes.reserve(gState.lastWrite.size() / 8);
    for (std::size_t offset = 0; offset < gState.lastWrite.size(); offset += 8) {
        uint8_t value = 0;
        for (std::size_t bit = 0; bit < 8; ++bit) {
            value <<= 1;
            if (gState.lastWrite[offset + bit].duration0 > 3) {
                value |= 1;
            }
        }
        bytes.push_back(value);
    }
    return bytes;
}

} // namespace fake_arduino

bool rmtInit(uint8_t pin, uint8_t, rmt_reserve_memsize_t memoryBlocks, uint32_t frequencyHz) {
    ++gState.initCalls;
    gState.lastPin = pin;
    gState.lastMemoryBlocks = memoryBlocks;
    gState.lastFrequencyHz = frequencyHz;
    return gState.initResult;
}

bool rmtDeinit(uint8_t pin) {
    ++gState.deinitCalls;
    gState.lastPin = pin;
    return true;
}

bool rmtWrite(uint8_t pin, const rmt_data_t *data, std::size_t symbolCount, uint32_t) {
    ++gState.writeCalls;
    gState.lastPin = pin;
    gState.lastWrite.assign(data, data + symbolCount);
    return true;
}

void delayMicroseconds(uint32_t microseconds) {
    ++gState.delayMicrosecondsCalls;
    gState.lastDelayMicroseconds = microseconds;
}

void fakeLogError(const char *message) {
    gState.lastError = message;
}
