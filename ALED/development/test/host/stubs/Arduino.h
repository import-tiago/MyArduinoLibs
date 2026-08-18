// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstddef>
#include <stdint.h>

#define ARDUINO_ARCH_ESP32 1
#define SOC_RMT_SUPPORTED 1
#define SOC_RMT_TX_CANDIDATES_PER_GROUP 4

enum rmt_reserve_memsize_t : uint8_t {
    RMT_MEM_NUM_BLOCKS_1 = 1,
    RMT_MEM_NUM_BLOCKS_2 = 2,
    RMT_MEM_NUM_BLOCKS_3 = 3,
    RMT_MEM_NUM_BLOCKS_4 = 4,
};

struct rmt_data_t {
    uint32_t duration0 = 0;
    uint32_t level0 = 0;
    uint32_t duration1 = 0;
    uint32_t level1 = 0;
    uint32_t val = 0;
};

constexpr uint8_t RMT_TX_MODE = 0;
constexpr uint32_t RMT_WAIT_FOR_EVER = UINT32_MAX;

bool rmtInit(uint8_t pin, uint8_t mode, rmt_reserve_memsize_t memoryBlocks, uint32_t frequencyHz);
bool rmtDeinit(uint8_t pin);
bool rmtWrite(uint8_t pin, const rmt_data_t *data, std::size_t symbolCount, uint32_t timeout);
void delayMicroseconds(uint32_t microseconds);
void fakeLogError(const char *message);

#define log_e(message) fakeLogError(message)
