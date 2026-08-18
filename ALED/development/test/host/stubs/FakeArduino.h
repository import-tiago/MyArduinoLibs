// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "Arduino.h"

#include <string>
#include <vector>

namespace fake_arduino {

struct State {
    bool initResult = true;
    uint32_t initCalls = 0;
    uint32_t deinitCalls = 0;
    uint32_t writeCalls = 0;
    uint32_t delayMicrosecondsCalls = 0;
    uint8_t lastPin = 0;
    rmt_reserve_memsize_t lastMemoryBlocks = RMT_MEM_NUM_BLOCKS_1;
    uint32_t lastFrequencyHz = 0;
    uint32_t lastDelayMicroseconds = 0;
    std::string lastError;
    std::vector<rmt_data_t> lastWrite;
};

State &state();
void reset();
std::vector<uint8_t> decodeLastWriteBytes();

} // namespace fake_arduino
