// sensormanager.cpp
#include "Sensors/SensorManager.h"
#include "Init/SystemInit.h"
#include "Communication/NetworkManager.h"
#include "HAL/I2CBus.h"
#include "DataBlock.h"
#include "Sensors/MPU6500/MPU6500_Handler.h"
#include "Sensors/MPU6500/MPU6500_Orientation_Handler.h"
#include <math.h>
#include "Communication/SerialPort.h"

extern volatile CommandId g_currentCommand;
extern volatile uint8_t g_statusFlags;

GlobalDataBlock g_mpuData;
LocalDataBlock l_mpuData;

SensorManager &SensorManager::instance()
{
    static SensorManager sm;
    return sm;
}

bool SensorManager::begin(uint8_t address, TwoWire &wireRef)
{
    _imuHandler = new MPU6500_Handler(wireRef, address);
    if (_imuHandler->begin())
    {
        SERIAL_DEBUG("[SensorMgr] MPU6500 handler created and initialized at 0x%02X.\n", address);
        _initialized = true;
        g_statusFlags |= static_cast<uint8_t>(StatusFlags::FLAG_IMU_OK);

        _orientationHandler = new Orientation_Handler(100.0f);
        _orientationHandler->begin();
        SERIAL_DEBUG("[SensorMgr] Orientation_Handler initialized (Madgwick filter started)\n");
    }
    else
    {
        SERIAL_DEBUG("[SensorMgr] MPU6500 init FAILED at 0x%02X\n", address);
        delete _imuHandler;
        _imuHandler = nullptr;
        _initialized = false;
        g_statusFlags &= ~static_cast<uint8_t>(StatusFlags::FLAG_IMU_OK);
    }
    return _initialized;
}

void SensorManager::update()
{
    if (_initialized && _imuHandler)
    {
        bool ok = _imuHandler->readData();
        if (ok && _orientationHandler)
        {
            bool orientOk = _orientationHandler->updateOrientation();
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

        static uint32_t lastPrint = 0;
        if (millis() - lastPrint > 1000)
        {
            SERIAL_DEBUG("[SensorMgr] readData=%d | accel=(%.2f, %.2f, %.2f) | gyro=(%.2f, %.2f, %.2f)\n",
                         ok ? 1 : 0,
                         l_mpuData.accelX, l_mpuData.accelY, l_mpuData.accelZ,
                         l_mpuData.gyroX, l_mpuData.gyroY, l_mpuData.gyroZ);
            lastPrint = millis();
        }
    }
}

void SensorManager::calibrate()
{
    if (_initialized && _imuHandler)
    {
        SERIAL_DEBUG("[SensorMgr] 🔧 Calibration started (hold device still)...\n");
        _imuHandler->calibrate();
        g_mpuData.isCalibrated = true;
        SERIAL_DEBUG("[SensorMgr] ✅ Calibration completed, isCalibrated=true\n");
    }
    else
    {
        SERIAL_DEBUG("[SensorMgr] ⚠️ Cannot calibrate: not initialized\n");
    }
}

// --- Геттеры IMU ---
float SensorManager::getQuatW() const { return l_mpuData.quatW; }
float SensorManager::getQuatX() const { return l_mpuData.quatX; }
float SensorManager::getQuatY() const { return l_mpuData.quatY; }
float SensorManager::getQuatZ() const { return l_mpuData.quatZ; }
float SensorManager::getRoll() const { return l_mpuData.roll; }
float SensorManager::getPitch() const { return l_mpuData.pitch; }
float SensorManager::getYaw() const { return l_mpuData.yaw; }
float SensorManager::getAccelX() const { return l_mpuData.accelX; }
float SensorManager::getAccelY() const { return l_mpuData.accelY; }
float SensorManager::getAccelZ() const { return l_mpuData.accelZ; }
float SensorManager::getGyroX() const { return l_mpuData.gyroX; }
float SensorManager::getGyroY() const { return l_mpuData.gyroY; }
float SensorManager::getGyroZ() const { return l_mpuData.gyroZ; }
float SensorManager::getMagX() const { return l_mpuData.magX; }
float SensorManager::getMagY() const { return l_mpuData.magY; }
float SensorManager::getMagZ() const { return l_mpuData.magZ; }
float SensorManager::getTemperature() const { return l_mpuData.temperature; }

// ★ НОВОЕ: Заглушки для энкодеров (заменить на реальные MT6816)
float SensorManager::getEncoderAngleX() const
{
    // TODO: Реальное чтение с MT6816 через SPI
    return 0.0f;
}

float SensorManager::getEncoderAngleY() const
{
    // TODO: Реальное чтение с MT6816 через SPI
    return 0.0f;
}

bool SensorManager::isInitialized() const
{
    return _initialized;
}

void SensorManager::startTask(uint8_t coreId, uint8_t priority)
{
    if (_initialized)
    {
        xTaskCreatePinnedToCore(taskEntry, "SensorTask", 8192, nullptr, priority, nullptr, coreId);
        SERIAL_DEBUG("[SensorMgr] Task started on Core %d with Priority %d\n", coreId, priority);
    }
    else
    {
        SERIAL_DEBUG("[SensorMgr] Cannot start task: not initialized.\n");
    }
}

void SensorManager::taskEntry(void *arg)
{
    uint32_t period = System_GetConfig().control.sensorPeriodMs;
    TickType_t lastWake = xTaskGetTickCount();
    for (;;)
    {
        SensorManager::instance().update();
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(period));
    }
}