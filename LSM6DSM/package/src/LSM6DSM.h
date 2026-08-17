#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <math.h>

class LSM6DSM {
public:
    enum IntPin { INT1, INT2 };
    enum IntEvent {
        INT_DRDY_XL  = 0x01,
        INT_DRDY_G   = 0x02,
        INT_BOOT     = 0x04,
        INT_FTH      = 0x08,
        INT_STEP     = 0x10,
        INT_WAKEUP   = 0x20,
        INT_TILT     = 0x40,
        INT_TIMER    = 0x80
    };

public:
    enum Ascale_t { AFS_2G, AFS_16G, AFS_4G, AFS_8G };
    enum Gscale_t { GFS_245DPS, GFS_500DPS, GFS_1000DPS, GFS_2000DPS };
    enum Rate_t {
        ODR_12_5Hz, ODR_26Hz, ODR_52Hz, ODR_104Hz,
        ODR_208Hz, ODR_416Hz, ODR_833Hz, ODR_1660Hz,
        ODR_3330Hz, ODR_6660Hz
    };
    enum Error_t { ERROR_NONE, ERROR_CONNECT, ERROR_ID, ERROR_SELFTEST };

    LSM6DSM();

    Error_t begin();
    void configure(Ascale_t ascale, Gscale_t gscale, Rate_t aodr, Rate_t godr,
                   float accelBias[3], float gyroBias[3]);

    void calibrate(float *gyroBias, float *accelBias);
    void clearInterrupt();
    bool checkNewData();
    void readData(float &ax, float &ay, float &az, float &gx, float &gy, float &gz);

    // Interrupt configuration
    void configureInterrupt(IntPin pin, IntEvent event, bool enable);
    void enableDataReadyInterruptOnINT1(bool accel, bool gyro);
    void enableWakeUpInterrupt(float threshold_g, uint8_t duration_odr_cycles, IntPin pin);
    
uint8_t readRegister(uint8_t reg);
private:
    static constexpr uint8_t ADDRESS = 0x6A;
    static constexpr uint8_t WHO_AM_I_REG = 0x0F;
    static constexpr uint8_t WHO_AM_I_EXPECTED = 0x6A;

    static constexpr float ACCEL_MIN = .09;
    static constexpr float ACCEL_MAX = 1.7;
    static constexpr float GYRO_MIN = 20;
    static constexpr float GYRO_MAX = 80;

    float _accelBias[3] = {0};
    float _gyroBias[3] = {0};
    float _ares = 0, _gres = 0;
    Ascale_t _ascale = AFS_2G;
    Gscale_t _gscale = GFS_245DPS;
    Rate_t _aodr = ODR_104Hz;
    Rate_t _godr = ODR_104Hz;

    void writeRegister(uint8_t reg, uint8_t data);
    
    void readRegisters(uint8_t reg, uint8_t count, uint8_t *dest);
    void readRawData(int16_t data[7]);
    bool selfTest();
    bool inBounds(int16_t ptest[3], int16_t ntest[3], int16_t nom[3], float res, float minval, float maxval);
    bool outOfBounds(float val, float minval, float maxval);
};
