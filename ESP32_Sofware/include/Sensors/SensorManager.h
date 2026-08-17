#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "Communication/TelemetryPacket.h"
#include "Init/ProjectConfig.h"

// Forward declarations
class MPU6500_Handler;
class Orientation_Handler;

// ============================================================
//  Единый блок данных ВСЕХ датчиков
//  (IMU, энкодеры, будущие датчики)
// ============================================================

struct EncoderData {
    float angleDeg    = 0.0f;   // Абсолютный угол (градусы)
    float velocityDps = 0.0f;   // Угловая скорость (град/с)
    uint16_t rawValue = 0;      // Сырое значение SPI
    bool valid        = false;  // Данные достоверны
};

struct SensorDataBlock {
    IMU_Data    imu;
    EncoderData encoderX;       // MT6816 ось X (Этап 6)
    EncoderData encoderY;       // MT6816 ось Y (Этап 6)
    uint32_t    lastUpdateMs = 0;
};

// ============================================================
//  SensorManager — единая точка сбора данных с датчиков
// ============================================================

class SensorManager {
private:
    // Аппаратная часть
    TwoWire              _wire;
    MPU6500_Handler*     _imuHandler;
    Orientation_Handler* _orientationHandler;

    // Хранилище данных
    SensorDataBlock           _data;
    mutable SemaphoreHandle_t _mutex;

    // Задача FreeRTOS
    TaskHandle_t _taskHandle;
    uint32_t     _updatePeriodMs;
    bool         _initialized;

    // Приватный конструктор (синглтон)
    SensorManager();

    // Внутренние методы чтения
    void readImu();
    void readEncoders();   // Этап 6: MT6816

    // Точка входа для FreeRTOS
    static void sensorTaskEntry(void* param);
    void sensorTaskLoop();

public:
    ~SensorManager();

    SensorManager(const SensorManager&) = delete;
    SensorManager& operator=(const SensorManager&) = delete;

    static SensorManager& instance();

    // --- Инициализация (вызывается из System_Init) ---
    bool begin(const ProjectConfig& config);

    // --- Запуск задачи опроса (вызывается из System_StartTasks) ---
    bool startTask(uint8_t coreId = 1, uint8_t priority = 2);

    // --- Потокобезопасное получение данных ---
    bool getImuData(IMU_Data& out) const;
    bool getSensorData(SensorDataBlock& out) const;

    // --- Удобные геттеры (для networkTask) ---
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

    // --- Калибровка ---
    bool calibrateImu(uint16_t samples);

    // --- Статус ---
    bool isReady() const;
};