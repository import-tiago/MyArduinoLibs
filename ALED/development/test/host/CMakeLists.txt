cmake_minimum_required(VERSION 3.16)

project(MyALEDHostTests LANGUAGES CXX)

enable_testing()

add_executable(
    myaled_host_tests
    stubs/Arduino.cpp
    test_myaled.cpp
    ../../../package/src/MyALED.cpp
    ../../../package/src/MyALEDGamma.cpp
)

target_include_directories(
    myaled_host_tests
    PRIVATE
        stubs
        ../../../package/src
)

target_compile_features(myaled_host_tests PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(myaled_host_tests PRIVATE /W4 /WX)
else()
    target_compile_options(myaled_host_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME myaled_host_tests COMMAND myaled_host_tests)
