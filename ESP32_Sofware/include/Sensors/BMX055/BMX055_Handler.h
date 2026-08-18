#pragma once

#include <Wire.h>
#include "DataBlock.h"

namespace bfs { class Bmx055; }

// ★ Вынесена ЗА пределы класса — фикс ошибки с дефолтным аргументом
struct BMX055_Addresses {
    uint8_t accel = 0x19;
    uint8_t gyro  = 0x68;
    uint8_t mag   = 0x10;
};

class BMX055_Handler {
public:
    BMX055_Handler(TwoWire& wire, const BMX055_Addresses& addrs = BMX055_Addresses());
    ~BMX055_Handler();

    bool begin();
    bool calibrate(uint16_t samples);
    bool readData();

private:
    TwoWire* _i2cBus;
    BMX055_Addresses _addrs;
    bfs::Bmx055* _imu;
};