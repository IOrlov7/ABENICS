#pragma once

#include <Wire.h>
#include "DataBlock.h"

namespace bfs { class Mpu6500; }

class MPU6500_Handler {
public:
    MPU6500_Handler(TwoWire& wire, uint8_t addr);
    ~MPU6500_Handler();

    bool begin();
    bool calibrate(uint16_t samples);
    bool readData();  // ★ Унифицированный метод: читает данные и пишет в l_mpuData

private:
    TwoWire* _i2cBus;
    uint8_t _i2cAddress;
    bfs::Mpu6500* _imu;
};