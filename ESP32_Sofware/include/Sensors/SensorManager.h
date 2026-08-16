#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <freertos/semphr.h>

#include "Init/ProjectConfig.h"
#include "Sensors/BMX055/BMX055_Handler.h"
#include "Sensors/MPU6500/MPU6500_Handler.h"

struct ImuData {
    float accelG[3] = {0.0f, 0.0f, 0.0f};
    float gyroDps[3] = {0.0f, 0.0f, 0.0f};
    float magRaw[3] = {0.0f, 0.0f, 0.0f};

    bool accelOk = false;
    bool gyroOk = false;
    bool magOk = false;

    uint32_t timestampMs = 0;

    OrientationSensor source = OrientationSensor::NONE;
};

class SensorManager {
public:
    static SensorManager& instance();

    bool begin(const ProjectConfig& cfg);

    // Читает данные с выбранного датчика и обновляет внутренний буфер.
    bool update();

    // Копия последних данных.
    bool getImu(ImuData& out);

private:
    SensorManager();
    ~SensorManager();

    bool readBmx(ImuData& out);
    bool readMpu(ImuData& out);

    void lock();
    void unlock();

    ProjectConfig _cfg;

    BMX055_Handler* _bmx055;
    MPU6500_Handler* _mpu6500;

    // Если у тебя MPU6500 сидит на другом адресе, поменяй на 0x69.
    uint8_t _mpu6500Address;

    ImuData _data;

    bool _ready;

    SemaphoreHandle_t _mutex;
};