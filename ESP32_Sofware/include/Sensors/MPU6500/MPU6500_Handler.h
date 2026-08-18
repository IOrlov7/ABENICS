#ifndef MPU6500_HANDLER_H
#define MPU6500_HANDLER_H

#include "DataBlock.h" // Для l_mpuData
#include <Arduino.h>
#include <Wire.h> // Для TwoWire

namespace bfs {
    class Mpu6500; // Предварительное объявление
}

class MPU6500_Handler {
public:
    // ★ ПРИНЯТЬ ССЫЛКУ НА TwoWire из I2CBus
    explicit MPU6500_Handler(TwoWire& wire, uint8_t addr = 0x68);

    ~MPU6500_Handler();

    bool begin();
    bool calibrate(uint16_t samples = 500);
    bool readData();

private:
    TwoWire* _i2cBus; // ★ ХРАНИТЬ ССЫЛКУ НА ШИНУ
    uint8_t _i2cAddress;
    bfs::Mpu6500* _imu;
};

#endif