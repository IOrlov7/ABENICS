#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "DataBlock.h" // Для GlobalDataBlock, LocalDataBlock
// ★ УБРАТЬ: #include "MPU6500/MPU6500_Handler.h" (только в .cpp)
// ★ УБРАТЬ: #include "ProjectConfig.h" (не нужен для сигнатуры begin)
#include <Arduino.h>
#include <Wire.h> // Для TwoWire
#include "Sensors/MPU6500/MPU6500_Handler.h"
#include "Sensors/MPU6500/MPU6500_Orientation_Handler.h" 

class MPU6500_Handler; // ★ Предварительное объявление

class SensorManager {
public:
    static SensorManager& instance();

    // ★ ИЗМЕНЁН: Принимает только адрес и ссылку на Wire
    bool begin(uint8_t address, TwoWire& wireRef);

    void update(); // Опрос датчика
    void calibrate(); // Калибровка

    // ★ Геттеры для NetworkManager
    float getQuatW() const;
    float getQuatX() const;
    float getQuatY() const;
    float getQuatZ() const;
    float getRoll() const;
    float getPitch() const;
    float getYaw() const;
    float getAccelX() const;
    float getAccelY() const;
    float getAccelZ() const;
    float getGyroX() const;
    float getGyroY() const;
    float getGyroZ() const;
    float getMagX() const;
    float getMagY() const;
    float getMagZ() const;
    float getTemperature() const;

    // ★ Запуск задачи
    void startTask(uint8_t coreId, uint8_t priority);

    // ★ Метод для проверки инициализации
    bool isInitialized() const;

private:
    SensorManager() = default; // Singleton
    static void taskEntry(void* arg);

    MPU6500_Handler* _imuHandler = nullptr;
    Orientation_Handler* _orientationHandler = nullptr; 
    // ★ УБРАТЬ: SensorType _type = SensorType::UNKNOWN;
    bool _initialized = false;
    // Добавьте сюда поля для фильтра Маджвика или других вычислений, если нужно
};

#endif // SENSOR_MANAGER_H