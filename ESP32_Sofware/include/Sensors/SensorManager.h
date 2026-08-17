#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "Sensors/MPU6500/MPU6500_Handler.h"
#include "Sensors/MPU6500/MPU6500_Orientation_Handler.h"
#include "Communication/TelemetryPacket.h"
#include "Init/ProjectConfig.h"
#include "DataBlock.h"

class SensorManager {
private:
    TwoWire _wire;
    MPU6500_Handler _imuHandler;
    Orientation_Handler _orientationHandler;

    IMU_Data _imuData;
    bool _initialized = false;
    
    // Мьютекс для защиты данных при чтении из разных ядер
    SemaphoreHandle_t _mutex;

    // Приватный конструктор (синглтон)
    SensorManager() : _wire(0), _imuHandler(_wire, 0x68), _orientationHandler(100.0f) {
        memset(&_imuData, 0, sizeof(IMU_Data));
        _mutex = xSemaphoreCreateMutex();
    }

public:
    // Запрет копирования
    SensorManager(const SensorManager&) = delete;
    SensorManager& operator=(const SensorManager&) = delete;

    ~SensorManager() {
        if (_mutex) vSemaphoreDelete(_mutex);
    }

    static SensorManager& instance() {
        static SensorManager inst;
        return inst;
    }

    bool begin(const ProjectConfig& config) {
        if (!_imuHandler.begin()) {
            Serial.println("[FATAL] MPU6500 init failed!");
            return false;
        }
        
        Serial.println("[...] Calibrating MPU6500...");
        _imuHandler.calibrate(1000);
        _orientationHandler.begin();
        
        _initialized = true;
        Serial.println("[OK] SensorManager initialized.");
        return true;
    }

    // Вызывается из sensorTask (Core 1)
    void update() {
        if (!_initialized) return;

        if (_imuHandler.readData()) {
            _orientationHandler.updateOrientation(); // Обновляет глобальную l_mpuData
            
            // Блокируем мьютекс перед записью в _imuData
            if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                _imuData.quat_w = l_mpuData.quatW;
                _imuData.quat_x = l_mpuData.quatX;
                _imuData.quat_y = l_mpuData.quatY;
                _imuData.quat_z = l_mpuData.quatZ;

                _imuData.roll  = l_mpuData.roll;
                _imuData.pitch = l_mpuData.pitch;
                _imuData.yaw   = l_mpuData.yaw;

                _imuData.accel_x = (int16_t)l_mpuData.accelX; 
                _imuData.accel_y = (int16_t)l_mpuData.accelY;
                _imuData.accel_z = (int16_t)l_mpuData.accelZ;

                _imuData.gyro_x = (int16_t)l_mpuData.gyroX;
                _imuData.gyro_y = (int16_t)l_mpuData.gyroY;
                _imuData.gyro_z = (int16_t)l_mpuData.gyroZ;

                _imuData.temperature = l_mpuData.temperature;
                
                // Магнитометра у MPU6500 нет
                _imuData.mag_x = 0; 
                _imuData.mag_y = 0; 
                _imuData.mag_z = 0;

                xSemaphoreGive(_mutex);
            }
        }
    }

    // Получить полную копию данных (используется в System_GetImuData)
    bool getImu(IMU_Data& out) const {
        if (!_initialized) return false;
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            out = _imuData;
            xSemaphoreGive(const_cast<SemaphoreHandle_t&>(_mutex)); // const_cast для const метода
            return true;
        }
        return false;
    }

    // --- Геттеры для networkTask (Core 0) ---
    float getQuatW() const { return _imuData.quat_w; }
    float getQuatX() const { return _imuData.quat_x; }
    float getQuatY() const { return _imuData.quat_y; }
    float getQuatZ() const { return _imuData.quat_z; }
    
    float getRoll() const { return _imuData.roll; }
    float getPitch() const { return _imuData.pitch; }
    float getYaw() const { return _imuData.yaw; }
    
    int16_t getAccelX() const { return _imuData.accel_x; }
    int16_t getAccelY() const { return _imuData.accel_y; }
    int16_t getAccelZ() const { return _imuData.accel_z; }
    
    int16_t getGyroX() const { return _imuData.gyro_x; }
    int16_t getGyroY() const { return _imuData.gyro_y; }
    int16_t getGyroZ() const { return _imuData.gyro_z; }
    
    int16_t getMagX() const { return 0; }
    int16_t getMagY() const { return 0; }
    int16_t getMagZ() const { return 0; }
    
    float getTemperature() const { return _imuData.temperature; }
};