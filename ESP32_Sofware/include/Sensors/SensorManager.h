#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "Communication/TelemetryPacket.h"
#include "Init/ProjectConfig.h"

class MPU6500_Handler;
class BMX055_Handler;
class Orientation_Handler;

struct EncoderData {
    float angleDeg    = 0.0f;
    float velocityDps = 0.0f;
    uint16_t rawValue = 0;
    bool valid        = false;
};

struct SensorDataBlock {
    IMU_Data    imu;
    EncoderData encoderX;
    EncoderData encoderY;
    uint32_t    lastUpdateMs = 0;
};

class SensorManager {
private:
    TwoWire _wire;
    
    // ★ Храним указатели на оба типа Handler'ов
    MPU6500_Handler* _mpu6500 = nullptr;
    BMX055_Handler*  _bmx055  = nullptr;
    
    Orientation_Handler* _orientationHandler = nullptr;

    SensorDataBlock           _data;
    mutable SemaphoreHandle_t _mutex;
    TaskHandle_t              _taskHandle;
    uint32_t                  _updatePeriodMs;
    bool                      _initialized;
    OrientationSensor         _currentImuType;

    SensorManager();

    void readImu();
    void readEncoders();
    
    static void sensorTaskEntry(void* param);
    void sensorTaskLoop();

public:
    ~SensorManager();

    SensorManager(const SensorManager&) = delete;
    SensorManager& operator=(const SensorManager&) = delete;

    static SensorManager& instance();

    bool begin(const ProjectConfig& config);
    bool startTask(uint8_t coreId = 1, uint8_t priority = 2);

    bool getImuData(IMU_Data& out) const;
    bool getSensorData(SensorDataBlock& out) const;

    float   getQuatW()  const;
    float   getQuatX()  const;
    float   getQuatY()  const;
    float   getQuatZ()  const;
    float   getRoll()   const;
    float   getPitch()  const;
    float   getYaw()    const;
    int16_t getAccelX() const;
    int16_t getAccelY() const;
    int16_t getAccelZ() const;
    int16_t getGyroX()  const;
    int16_t getGyroY()  const;
    int16_t getGyroZ()  const;
    int16_t getMagX()   const;
    int16_t getMagY()   const;
    int16_t getMagZ()   const;
    float   getTemperature() const;

    bool calibrateImu(uint16_t samples);
    bool isReady() const;
    const char* getCurrentImuName() const;
};