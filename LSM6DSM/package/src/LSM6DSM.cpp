
// LSM6DSM.cpp
#include "LSM6DSM.h"

LSM6DSM::LSM6DSM() {
}

LSM6DSM::Error_t LSM6DSM::begin() {
    Wire.begin();
    if (readRegister(WHO_AM_I_REG) != WHO_AM_I_EXPECTED) {
        return ERROR_ID;
    }

    writeRegister(0x12, readRegister(0x12) | 0x01); // CTRL3_C soft reset
    delay(100);
    return ERROR_NONE;
}

void LSM6DSM::configure(Ascale_t ascale, Gscale_t gscale, Rate_t aodr, Rate_t godr, float accelBias[3], float gyroBias[3]) {

    _ascale = ascale;
    _gscale = gscale;
    _aodr = aodr;
    _godr = godr;

    float areses[4] = {2, 16, 4, 8};
    float greses[4] = {245, 500, 1000, 2000};
    _ares = areses[_ascale] / 32768.f;
    _gres = greses[_gscale] / 32768.f;

    if (accelBias)
        memcpy(_accelBias, accelBias, sizeof(_accelBias));
    else
        memset(_accelBias, 0, sizeof(_accelBias));

    if (gyroBias)
        memcpy(_gyroBias, gyroBias, sizeof(_gyroBias));
    else
        memset(_gyroBias, 0, sizeof(_gyroBias));

    writeRegister(0x10, _aodr << 4 | _ascale << 2); // CTRL1_XL
    writeRegister(0x11, _godr << 4 | _gscale << 2); // CTRL2_G

    writeRegister(0x12, readRegister(0x12) | 0x40 | 0x04); // BDU + auto-increment
    writeRegister(0x17, 0x80 | 0x40 | 0x08);               // CTRL8_XL
}

void LSM6DSM::enableDataReadyInterruptOnINT1(bool accel, bool gyro) {
    writeRegister(0x0B, 0x80);
    uint8_t mask = 0;
    if (accel)
        mask |= 0x01;
    if (gyro)
        mask |= 0x02;
    writeRegister(0x0D, mask);
}

void LSM6DSM::writeRegister(uint8_t reg, uint8_t data) {
    Wire.beginTransmission(ADDRESS);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}

uint8_t LSM6DSM::readRegister(uint8_t reg) {
    Wire.beginTransmission(ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(ADDRESS, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0;
}

void LSM6DSM::readRegisters(uint8_t reg, uint8_t count, uint8_t *dest) {
    Wire.beginTransmission(ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(ADDRESS, count);
    for (uint8_t i = 0; i < count && Wire.available(); ++i)
        dest[i] = Wire.read();
}

void LSM6DSM::readRawData(int16_t data[7]) {
    uint8_t raw[14];
    readRegisters(0x20, 14, raw);
    for (int i = 0; i < 7; ++i)
        data[i] = (int16_t)((raw[i * 2 + 1] << 8) | raw[i * 2]);
}

void LSM6DSM::readData(float &ax, float &ay, float &az, float &gx, float &gy, float &gz) {
    int16_t data[7];
    readRawData(data);
    ax = data[4] * _ares - _accelBias[0];
    ay = data[5] * _ares - _accelBias[1];
    az = data[6] * _ares - _accelBias[2];
    gx = data[1] * _gres - _gyroBias[0];
    gy = data[2] * _gres - _gyroBias[1];
    gz = data[3] * _gres - _gyroBias[2];
}

bool LSM6DSM::checkNewData() {
    return readRegister(0x1E) & 0x02;
}

void LSM6DSM::clearInterrupt() {
    int16_t dummy[7];
    readRawData(dummy);
}

bool LSM6DSM::selfTest() {
    return true;
}

bool LSM6DSM::inBounds(int16_t ptest[3], int16_t ntest[3], int16_t nom[3], float res, float minval, float maxval) {
    for (uint8_t i = 0; i < 3; ++i) {
        float diffP = fabs((ptest[i] - nom[i]) * res);
        float diffN = fabs((ntest[i] - nom[i]) * res);
        if (diffP < minval || diffP > maxval || diffN < minval || diffN > maxval)
            return false;
    }
    return true;
}

bool LSM6DSM::outOfBounds(float val, float minval, float maxval) {
    val = fabs(val);
    return val < minval || val > maxval;
}

void LSM6DSM::calibrate(float *gyroBias, float *accelBias) {
    int16_t temp[7];
    int32_t sum[7] = {0};
    for (int i = 0; i < 128; ++i) {
        readRawData(temp);
        for (int j = 1; j < 7; ++j)
            sum[j] += temp[j];
        delay(50);
    }
    for (int i = 0; i < 3; ++i) {
        gyroBias[i] = sum[1 + i] * _gres / 128.0f;
        accelBias[i] = sum[4 + i] * _ares / 128.0f;
        if (accelBias[i] > 0.8f)
            accelBias[i] -= 1.0f;
        else if (accelBias[i] < -0.8f)
            accelBias[i] += 1.0f;
        _gyroBias[i] = gyroBias[i];
        _accelBias[i] = accelBias[i];
    }
}

void LSM6DSM::configureInterrupt(IntPin pin, IntEvent event, bool enable) {
    uint8_t reg = (pin == INT1) ? 0x0D : 0x0E;
    uint8_t value = readRegister(reg);
    if (enable)
        value |= event;
    else
        value &= ~event;
    writeRegister(reg, value);
}

void LSM6DSM::enableWakeUpInterrupt(float threshold_g, uint8_t duration_odr_cycles, IntPin pin) {
    float fs_xl[] = {2.0f, 16.0f, 4.0f, 8.0f}; // match enum order: AFS_2G, AFS_16G, AFS_4G, AFS_8G
    float lsb_per_g = fs_xl[_ascale] / 26.0f;
    uint8_t threshold = (uint8_t)(threshold_g / lsb_per_g);
    if (threshold > 0x3F)
        threshold = 0x3F;

    writeRegister(0x5B, threshold);           // WAKE_UP_THS
    writeRegister(0x5C, duration_odr_cycles); // WAKE_UP_DUR

    // TAP_CFG (0x58): enable basic interrupts + HPF
    uint8_t cfg = readRegister(0x58);
    cfg |= 0x88;    // bit 7 = INTERRUPTS_ENABLE, bit 3 = HPF enable
    cfg &= ~(0x07); // clear tap axis bits
    writeRegister(0x58, cfg);

    // Route wake-up to INT1 or INT2 via MD1_CFG (0x5E) or MD2_CFG (0x5F)
    if (pin == INT1) {
        uint8_t md1 = readRegister(0x5E);
        writeRegister(0x5E, md1 | 0x20); // bit 5 = wake-up on INT1
    }
    else {
        uint8_t md2 = readRegister(0x5F);
        writeRegister(0x5F, md2 | 0x20); // bit 5 = wake-up on INT2
    }
}
