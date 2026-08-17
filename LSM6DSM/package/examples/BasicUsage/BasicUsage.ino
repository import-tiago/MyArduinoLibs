
#include <LSM6DSM.h>
#include <Arduino.h>
#include <Wire.h>

#define LED_PIN 39
#define IMU_SA0 16
#define IMU_INT_PIN 17
#define IMU_CS 18
#define IMU_INT2_PIN 15

LSM6DSM imu;

volatile bool motionDetected = false;

void IRAM_ATTR onMotionInterrupt() {
    motionDetected = true;
}

void setup(void) {

    Serial.begin(19200);

    pinMode(LED_PIN, OUTPUT);

    pinMode(IMU_SA0, OUTPUT);
    pinMode(IMU_INT_PIN, INPUT_PULLUP);
    pinMode(IMU_INT2_PIN, INPUT_PULLUP);
    pinMode(IMU_CS, OUTPUT);

    digitalWrite(IMU_SA0, LOW); // LOW = 0X6A, HIGH = 0X6B
    digitalWrite(IMU_CS, HIGH); // HIGH = I2C, LOW = SPI
    digitalWrite(LED_PIN, LOW);

    Wire.begin(48, 47); // SDA, SCL

    attachInterrupt(IMU_INT_PIN, onMotionInterrupt, RISING);

    if (imu.begin() != LSM6DSM::ERROR_NONE) {
        Serial.println("IMU not found.");
        while (1);
    }

    float accelBias[3] = {0.0, 0.0, 0.0};
    float gyroBias[3] = {0.0, 0.0, 0.0};

    imu.configure(
        LSM6DSM::AFS_2G,
        LSM6DSM::GFS_1000DPS,
        LSM6DSM::ODR_208Hz,
        LSM6DSM::ODR_208Hz,
        accelBias,
        gyroBias);

    imu.enableWakeUpInterrupt(0.1, 1, LSM6DSM::INT1);  // threshold = 0.1g, duration = 1 ODR cycle, routed to INT1
}

void loop() {

    float ax, ay, az, gx, gy, gz;
    imu.readData(ax, ay, az, gx, gy, gz);
    Serial.print(ax);
    Serial.print(",");
    Serial.print(ay);
    Serial.print(",");
    Serial.println(az);

    if (motionDetected) {
        motionDetected = false;
        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);
    }

    delay(50);
}
