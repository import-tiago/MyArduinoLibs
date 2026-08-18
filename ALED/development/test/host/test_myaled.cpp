// SPDX-License-Identifier: GPL-3.0-only

#include "FakeArduino.h"
#include "MyALED.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <new>
#include <string>
#include <type_traits>
#include <vector>

namespace {

int gNothrowArrayAllocationsBeforeFailure = -1;

} // namespace

void *operator new[](std::size_t size) {
    if (void *memory = std::malloc(size)) {
        return memory;
    }
    throw std::bad_alloc();
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
    if (gNothrowArrayAllocationsBeforeFailure == 0) {
        gNothrowArrayAllocationsBeforeFailure = -1;
        return nullptr;
    }
    if (gNothrowArrayAllocationsBeforeFailure > 0) {
        --gNothrowArrayAllocationsBeforeFailure;
    }
    return std::malloc(size);
}

void operator delete[](void *memory) noexcept {
    std::free(memory);
}

void operator delete[](void *memory, std::size_t) noexcept {
    std::free(memory);
}

namespace {

int gFailures = 0;

void check(bool condition, const char *expression, const char *file, int line) {
    if (condition) {
        return;
    }

    ++gFailures;
    std::cerr << file << ':' << line << ": check failed: " << expression << '\n';
}

#define CHECK(expression) check((expression), #expression, __FILE__, __LINE__)

void checkBytes(const std::vector<uint8_t> &actual, std::initializer_list<uint8_t> expected) {
    CHECK(actual == std::vector<uint8_t>(expected));
}

void testTypeContract() {
    static_assert(!std::is_default_constructible<MyALED>::value, "A strip must be configured at construction");
    static_assert(!std::is_copy_constructible<MyALED>::value, "Buffer ownership must not be copied");
    static_assert(!std::is_move_constructible<MyALED>::value, "RMT ownership must not be moved");
    static_assert(noexcept(myaled::packColor(0, 0, 0)), "Color helpers must not throw");
    static_assert(noexcept(std::declval<const MyALED &>().isValid()), "State queries must not throw");
    static_assert(noexcept(std::declval<const MyALED &>().getPixelColor(0)), "Pixel queries must not throw");

    static_assert(static_cast<uint8_t>(LedType::SK6812_WRGB) == 10, "Stable LED type value");
    static_assert(static_cast<uint8_t>(LedType::SK6812_WGRB) == 11, "Stable LED type value");
    static_assert(static_cast<uint8_t>(LedType::WS2812_RGB) == 20, "Stable LED type value");
    static_assert(static_cast<uint8_t>(LedType::WS2812_GRB) == 21, "Stable LED type value");
    static_assert(SK6812WRGB == LedType::SK6812_WRGB, "Legacy value alias");
    static_assert(WS2812GRB == LedType::WS2812_GRB, "Legacy value alias");
}

void testPackedColorContract() {
    constexpr uint32_t packed = MyALED::Color(0x11, 0x22, 0x33, 0x44);
    static_assert(packed == 0x44112233, "Packed color layout must be 0xWWRRGGBB");
    static_assert(MyALED::getWhite(packed) == 0x44, "White byte");
    static_assert(MyALED::getRed(packed) == 0x11, "Red byte");
    static_assert(MyALED::getGreen(packed) == 0x22, "Green byte");
    static_assert(MyALED::getBlue(packed) == 0x33, "Blue byte");
    static_assert(MyALED::makeRGBWcolor(0x11, 0x22, 0x33, 0x44) == packed, "Equivalent packers");

    constexpr RGBW unpacked = myaled::unpackColor(packed);
    static_assert(unpacked.r == 0x11, "Logical RGBW field order");
    static_assert(unpacked.g == 0x22, "Logical RGBW field order");
    static_assert(unpacked.b == 0x33, "Logical RGBW field order");
    static_assert(unpacked.w == 0x44, "Logical RGBW field order");
}

void testInitializationAndLifetime() {
    fake_arduino::reset();
    {
        MyALED strip(4, 7, LedType::WS2812_GRB);
        CHECK(strip.isValid());
        CHECK(!strip.isBegun());
        CHECK(strip.begin(RMT_MEM_NUM_BLOCKS_2));
        CHECK(strip.isBegun());
        CHECK(fake_arduino::state().initCalls == 1);
        CHECK(fake_arduino::state().lastPin == 7);
        CHECK(fake_arduino::state().lastMemoryBlocks == RMT_MEM_NUM_BLOCKS_2);
        CHECK(fake_arduino::state().lastFrequencyHz == 10000000);

        CHECK(strip.begin());
        CHECK(fake_arduino::state().initCalls == 1);
    }
    CHECK(fake_arduino::state().deinitCalls == 1);

    fake_arduino::reset();
    fake_arduino::state().initResult = false;
    MyALED failedInit(1, 8, LedType::WS2812_RGB);
    CHECK(!failedInit.begin());
    CHECK(!failedInit.isBegun());
    CHECK(fake_arduino::state().lastError == "RMT initialization failed");
    fake_arduino::state().initResult = true;
    CHECK(failedInit.begin());

    MyALED invalidType(1, 8, static_cast<LedType>(0xFF));
    CHECK(!invalidType.isValid());
    CHECK(!invalidType.begin());

    MyALED empty(0, 8, LedType::WS2812_RGB);
    CHECK(!empty.isValid());
    CHECK(!empty.begin());
}

void testPixelBufferOperations() {
    MyALED strip(4, 8, LedType::WS2812_RGB);
    CHECK(strip.isValid());
    CHECK(strip.numPixels() == 4);
    CHECK(strip.getBrightness() == 128);

    const uint32_t first = MyALED::Color(0x11, 0x22, 0x33, 0x44);
    strip.setPixelColor(0, first);
    CHECK(strip.getPixelColor(0) == first);

    const RGBW second{0x55, 0x66, 0x77, 0x88};
    strip.setPixelColor(1, second);
    CHECK(strip.getPixelColor(1) == MyALED::Color(0x55, 0x66, 0x77, 0x88));

    strip.setPixelColor(2, 0x99, 0xAA, 0xBB, 0xCC);
    CHECK(strip.getPixelColor(2) == MyALED::Color(0x99, 0xAA, 0xBB, 0xCC));

    strip.setPixelColor(99, MyALED::Color(1, 2, 3));
    CHECK(strip.getPixelColor(99) == 0);

    const uint32_t fillColor = MyALED::Color(9, 8, 7, 6);
    strip.fill(fillColor, 1, 2);
    CHECK(strip.getPixelColor(0) == first);
    CHECK(strip.getPixelColor(1) == fillColor);
    CHECK(strip.getPixelColor(2) == fillColor);

    const RGBW tailColor{1, 2, 3, 4};
    strip.fill(tailColor, 3, std::numeric_limits<uint16_t>::max());
    CHECK(strip.getPixelColor(3) == MyALED::Color(1, 2, 3, 4));

    strip.clear();
    for (uint16_t index = 0; index < strip.numPixels(); ++index) {
        CHECK(strip.getPixelColor(index) == 0);
    }
}

void testDirtyTrackingAndResize() {
    fake_arduino::reset();
    MyALED strip(2, 8, LedType::WS2812_RGB);
    CHECK(strip.begin());
    strip.setBrightness(255);
    strip.setPixelColor(0, MyALED::Color(1, 2, 3));
    strip.show();
    CHECK(fake_arduino::state().writeCalls == 1);

    strip.show();
    CHECK(fake_arduino::state().writeCalls == 1);
    strip.setBrightness(255);
    strip.show();
    CHECK(fake_arduino::state().writeCalls == 1);
    strip.setBrightness(254);
    strip.show();
    CHECK(fake_arduino::state().writeCalls == 2);

    CHECK(strip.updateLength(4));
    CHECK(strip.numPixels() == 4);
    CHECK(strip.isValid());
    CHECK(strip.isBegun());
    CHECK(fake_arduino::state().writeCalls == 3); // Clears the previous physical range.
    for (uint16_t index = 0; index < strip.numPixels(); ++index) {
        CHECK(strip.getPixelColor(index) == 0);
    }

    CHECK(strip.updateLength(0));
    CHECK(strip.numPixels() == 0);
    CHECK(!strip.isValid());
    CHECK(strip.isBegun());
    CHECK(strip.updateLength(1));
    CHECK(strip.isValid());
    CHECK(strip.begin());
    CHECK(fake_arduino::state().initCalls == 1);
}

void testTransactionalAllocationFailure() {
    MyALED strip(2, 8, LedType::WS2812_RGB);
    const uint32_t first = MyALED::Color(1, 2, 3, 4);
    const uint32_t second = MyALED::Color(5, 6, 7, 8);
    strip.setPixelColor(0, first);
    strip.setPixelColor(1, second);

    gNothrowArrayAllocationsBeforeFailure = 0;
    CHECK(!strip.updateLength(4));
    CHECK(strip.numPixels() == 2);
    CHECK(strip.getPixelColor(0) == first);
    CHECK(strip.getPixelColor(1) == second);

    gNothrowArrayAllocationsBeforeFailure = 1;
    CHECK(!strip.updateLength(4));
    CHECK(strip.numPixels() == 2);
    CHECK(strip.getPixelColor(0) == first);
    CHECK(strip.getPixelColor(1) == second);

    gNothrowArrayAllocationsBeforeFailure = -1;
    CHECK(strip.updateLength(4));
    CHECK(strip.numPixels() == 4);
}

void checkWireOrder(LedType type, std::initializer_list<uint8_t> expected) {
    fake_arduino::reset();
    MyALED strip(1, 8, type);
    CHECK(strip.begin());
    strip.setBrightness(255);
    strip.setPixelColor(0, RGBW{0x11, 0x22, 0x33, 0x44});
    strip.show();
    checkBytes(fake_arduino::decodeLastWriteBytes(), expected);
}

void testProtocolByteOrder() {
    checkWireOrder(LedType::WS2812_RGB, {0x11, 0x22, 0x33});
    checkWireOrder(LedType::WS2812_GRB, {0x22, 0x11, 0x33});
    checkWireOrder(LedType::SK6812_WRGB, {0x44, 0x11, 0x22, 0x33});
    checkWireOrder(LedType::SK6812_WGRB, {0x44, 0x22, 0x11, 0x33});
}

void testBrightnessAndGamma() {
    fake_arduino::reset();
    MyALED strip(1, 8, LedType::WS2812_RGB);
    CHECK(strip.begin());
    strip.setBrightness(255);
    strip.setGamma(true);
    strip.setPixelColor(0, RGBW{128, 0, 0, 0});
    strip.show();
    checkBytes(fake_arduino::decodeLastWriteBytes(), {51, 0, 0});

    strip.setGamma(false);
    strip.setBrightness(128);
    strip.show();
    checkBytes(fake_arduino::decodeLastWriteBytes(), {64, 0, 0});
}

} // namespace

int main() {
    testTypeContract();
    testPackedColorContract();
    testInitializationAndLifetime();
    testPixelBufferOperations();
    testDirtyTrackingAndResize();
    testTransactionalAllocationFailure();
    testProtocolByteOrder();
    testBrightnessAndGamma();

    if (gFailures != 0) {
        std::cerr << gFailures << " test(s) failed\n";
        return 1;
    }

    std::cout << "All MyALED host tests passed\n";
    return 0;
}
