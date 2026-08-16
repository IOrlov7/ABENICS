#ifndef MPU6500_HANDLER_H
#define MPU6500_HANDLER_H

#include <Arduino.h>
#include <Wire.h>
#include "MPU6500.h" // bolderflight/invensense-imu
#include "DataBlock.h" // ← ИСПРАВЛЕНО: было DataStructures.h

class MPU6500_Handler {
public:
    MPU6500_Handler(TwoWire& wire, uint8_t addr);
    
    bool begin();
    bool calibrate(uint16_t samples);
    bool readData();

private:
    bfs::Mpu6500 imu;
    TwoWire* i2cBus;
    uint8_t i2cAddress;
    
    // НОВЫЙ МЕТОД: Запись смещений в аппаратные регистры
    void writeHardwareOffsets(int16_t accelOffsetX, int16_t accelOffsetY, int16_t accelOffsetZ,
                               int16_t gyroOffsetX, int16_t gyroOffsetY, int16_t gyroOffsetZ);
};

#endif