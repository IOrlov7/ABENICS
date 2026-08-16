#pragma once

#include <Arduino.h>

struct BMX055Data {
    float accelG[3] = {0.0f, 0.0f, 0.0f};
    float gyroDps[3] = {0.0f, 0.0f, 0.0f};
    int16_t magRaw[3] = {0, 0, 0};

    bool accelOk = false;
    bool gyroOk = false;
    bool magOk = false;

    uint32_t timestampMs = 0;
};

class BMX055_Handler {
public:
    bool begin();

    // Только чтение данных.
    // Никаких управляющих команд здесь быть не должно.
    bool read(BMX055Data& data);

private:
    bool initAccel();
    bool initGyro();
    bool initMag();

    bool readAccel(float accelG[3]);
    bool readGyro(float gyroDps[3]);
    bool readMagRaw(int16_t magRaw[3]);

    uint8_t _addrAccel = 0x18;
    uint8_t _addrGyro = 0x68;
    uint8_t _addrMag = 0x10;

    bool _accelPresent = false;
    bool _gyroPresent = false;
    bool _magPresent = false;
};