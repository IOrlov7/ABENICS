// sensormanager.cpp

#include "Sensors/SensorManager.h"
#include "Init/SystemInit.h"                 // Для System_GetConfig
#include "Communication/NetworkManager.h"    // Для g_statusFlags
#include "HAL/I2CBus.h"                      // Для получения ссылки на Wire
#include "DataBlock.h"                       // Для l_mpuData и g_mpuData
#include "Sensors/MPU6500/MPU6500_Handler.h" // Теперь включаем здесь
#include "Sensors/MPU6500/MPU6500_Orientation_Handler.h"
#include <math.h> // Для вычислений углов (если нужно)
#include "Communication/SerialPort.h"

// Используем внешние глобальные переменные из DataBlock.h
// Глобальные переменные состояния (определены в SystemInit.cpp)
extern volatile CommandId g_currentCommand;
extern volatile uint8_t g_statusFlags;

// ★ РЕАЛЬНЫЕ ОПРЕДЕЛЕНИЯ (выделение памяти) для структур IMU
GlobalDataBlock g_mpuData;
LocalDataBlock l_mpuData;

// Используем фильтр Маджвика из Orientation.h (предполагается, что он уже инициализирован)
// extern MadgwickFilter madgwickFilter; // Если используется

SensorManager &SensorManager::instance()
{
    static SensorManager sm;
    return sm;
}

// ИЗМЕНЁННЫЙ begin()
bool SensorManager::begin(uint8_t address, TwoWire &wireRef)
{
    // УПРОЩЁННО: всегда создаём MPU6500_Handler
    _imuHandler = new MPU6500_Handler(wireRef, address); // Передаём ссылку
    if (_imuHandler->begin())
    {
        // ★ ЗАМЕНЕНО: теперь используем макрос SERIAL_DEBUG
        SERIAL_DEBUG("[SensorMgr] MPU6500 handler created and initialized at 0x%02X.\n", address);
        _initialized = true; // Устанавливаем флаг инициализации
        // Устанавливаем флаг исправности IMU
        g_statusFlags |= static_cast<uint8_t>(StatusFlags::FLAG_IMU_OK);

        _orientationHandler = new Orientation_Handler(100.0f); // 100 Hz
        _orientationHandler->begin();
        SERIAL_DEBUG("[SensorMgr] Orientation_Handler initialized (Madgwick filter started)\n");
    }
    else
    {
        // ★ ЗАМЕНЕНО: теперь используем макрос SERIAL_DEBUG
        SERIAL_DEBUG("[SensorMgr] MPU6500 init FAILED at 0x%02X\n", address);
        delete _imuHandler;
        _imuHandler = nullptr;
        _initialized = false; // Устанавливаем флаг неудачи
        // Сбрасываем флаг исправности IMU
        g_statusFlags &= ~static_cast<uint8_t>(StatusFlags::FLAG_IMU_OK);
    }
    return _initialized;
}

void SensorManager::update()
{
    if (_initialized && _imuHandler)
    {
        // ★ Добавим проверку результата readData()
        bool ok = _imuHandler->readData();

        if (ok && _orientationHandler)
        {
            bool orientOk = _orientationHandler->updateOrientation();

            // 🔴 ДИАГНОСТИКА: Если updateOrientation() вернул false
            if (!orientOk)
            {
                static uint32_t lastWarn = 0;
                if (millis() - lastWarn > 2000)
                {
                    SERIAL_DEBUG("[SensorMgr] ⚠️ updateOrientation() FAILED: isError=%d, isCalibrated=%d\n",
                                 l_mpuData.isError, g_mpuData.isCalibrated);
                    lastWarn = millis();
                }
            }
        }

        // ★ Отладка раз в секунду
        static uint32_t lastPrint = 0;
        if (millis() - lastPrint > 1000)
        {
            // ★ ЗАМЕНЕНО: теперь используем макрос SERIAL_DEBUG
            SERIAL_DEBUG("[SensorMgr] readData=%d | accel=(%.2f, %.2f, %.2f) | gyro=(%.2f, %.2f, %.2f)\n",
                         ok ? 1 : 0,
                         l_mpuData.accelX, l_mpuData.accelY, l_mpuData.accelZ,
                         l_mpuData.gyroX, l_mpuData.gyroY, l_mpuData.gyroZ);
            lastPrint = millis();
        }
    }
}

void SensorManager::calibrate() {
    if (_initialized && _imuHandler) {
        SERIAL_DEBUG("[SensorMgr] 🔧 Calibration started (hold device still)...\n");
        _imuHandler->calibrate();
        
        // 🔴 ДОБАВИТЬ: Устанавливаем флаг калибровки
        g_mpuData.isCalibrated = true;
        
        SERIAL_DEBUG("[SensorMgr] ✅ Calibration completed, isCalibrated=true\n");
    } else {
        SERIAL_DEBUG("[SensorMgr] ⚠️ Cannot calibrate: not initialized\n");
    }
}

// --- Реализация геттеров ---
// Возвращают данные из l_mpuData (текущие) или g_mpuData (калибровочные), в зависимости от назначения

// ★ ИСПРАВЛЕНО: camelCase
float SensorManager::getQuatW() const { return l_mpuData.quatW; } // ★ ИСПРАВЛЕНО
float SensorManager::getQuatX() const { return l_mpuData.quatX; } // ★ ИСПРАВЛЕНО
float SensorManager::getQuatY() const { return l_mpuData.quatY; } // ★ ИСПРАВЛЕНО
float SensorManager::getQuatZ() const { return l_mpuData.quatZ; } // ★ ИСПРАВЛЕНО

float SensorManager::getRoll() const { return l_mpuData.roll; } // Угол в градусах
float SensorManager::getPitch() const { return l_mpuData.pitch; }
float SensorManager::getYaw() const { return l_mpuData.yaw; }

float SensorManager::getAccelX() const { return l_mpuData.accelX; }
float SensorManager::getAccelY() const { return l_mpuData.accelY; }
float SensorManager::getAccelZ() const { return l_mpuData.accelZ; }

float SensorManager::getGyroX() const { return l_mpuData.gyroX; }
float SensorManager::getGyroY() const { return l_mpuData.gyroY; }
float SensorManager::getGyroZ() const { return l_mpuData.gyroZ; }

float SensorManager::getMagX() const { return l_mpuData.magX; } // 0 для MPU6500
float SensorManager::getMagY() const { return l_mpuData.magY; }
float SensorManager::getMagZ() const { return l_mpuData.magZ; }

float SensorManager::getTemperature() const { return l_mpuData.temperature; }

bool SensorManager::isInitialized() const
{
    return _initialized;
}

void SensorManager::startTask(uint8_t coreId, uint8_t priority)
{
    if (_initialized)
    {
        xTaskCreatePinnedToCore(taskEntry, "SensorTask", 8192, nullptr, priority, nullptr, coreId);
        // ★ ЗАМЕНЕНО: теперь используем макрос SERIAL_DEBUG
        SERIAL_DEBUG("[SensorMgr] Task started on Core %d with Priority %d\n", coreId, priority);
    }
    else
    {
        // ★ ЗАМЕНЕНО: теперь используем макрос SERIAL_DEBUG
        SERIAL_DEBUG("[SensorMgr] Cannot start task: not initialized.\n");
    }
}

void SensorManager::taskEntry(void *arg)
{
    // ★ ИСПРАВЛЕНО: используем System_GetConfig().control.sensorPeriodMs
    uint32_t period = System_GetConfig().control.sensorPeriodMs; // ★ ИСПРАВЛЕНО: sensorPeriodMs из ControlConfig
    TickType_t lastWake = xTaskGetTickCount();

    for (;;)
    {
        SensorManager::instance().update();
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(period));
    }
}
