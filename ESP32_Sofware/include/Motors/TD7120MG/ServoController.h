#pragma once

#include <Arduino.h>

class ServoController {
public:
    bool begin(
        uint8_t address = 0x40,
        float frequencyHz = 50.0f,
        uint16_t minUs = 500,
        uint16_t maxUs = 2500,
        uint8_t maxChannels = 16
    );

    // Управление сервоприводами.
    bool setServoAngle(uint8_t channel, float angleDeg);
    bool setServoMicroseconds(uint8_t channel, uint16_t pulseUs);
    bool setServoOff(uint8_t channel);
    bool setAllOff();

    // Диагностическое чтение MODE1 PCA9685.
    // Это не чтение положения сервопривода.
    bool readMode1(uint8_t& mode1);

private:
    bool setFrequency(float frequencyHz);
    bool setPWM(uint8_t channel, uint16_t on, uint16_t off);
    uint16_t microsecondsToTicks(uint16_t us) const;

    uint8_t _address = 0x40;
    float _frequencyHz = 50.0f;

    uint16_t _minUs = 500;
    uint16_t _maxUs = 2500;

    uint8_t _maxChannels = 16;

    bool _ready = false;
};