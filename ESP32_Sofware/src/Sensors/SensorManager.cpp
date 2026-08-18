#include "Sensors/SensorManager.h"
#include "Sensors/MPU6500/MPU6500_Handler.h"
#include "Sensors/BMX055/BMX055_Handler.h"
#include "Sensors/MPU6500/MPU6500_Orientation_Handler.h"
#include "DataBlock.h"

GlobalDataBlock g_mpuData;
LocalDataBlock  l_mpuData;

SensorManager::SensorManager()
    : _wire(0), _mutex(nullptr), _taskHandle(nullptr), _updatePeriodMs(10), _initialized(false), _currentImuType(OrientationSensor::NONE)
{
    memset(&_data, 0, sizeof(SensorDataBlock));
    _mutex = xSemaphoreCreateMutex();
}

SensorManager::~SensorManager()
{
    if (_taskHandle)
        vTaskDelete(_taskHandle);
    if (_mutex)
        vSemaphoreDelete(_mutex);
    delete _mpu6500;
    delete _bmx055;
    delete _orientationHandler;
}

SensorManager &SensorManager::instance()
{
    static SensorManager inst;
    return inst;
}

bool SensorManager::begin(const ProjectConfig &config)
{
    _currentImuType = config.orientationSensor;

    // ★ Создаём нужный Handler на основе конфига
    switch (_currentImuType)
    {
    case OrientationSensor::MPU6500:
    {
        Serial.println("[SensorMgr] Creating MPU6500 handler");
        _mpu6500 = new MPU6500_Handler(_wire, config.imu.mpu6500Address);
        if (!_mpu6500->begin())
        {
            Serial.println("[SensorMgr] MPU6500 init FAILED");
            return false;
        }
        Serial.println("[SensorMgr] Calibrating MPU6500...");
        _mpu6500->calibrate(1000);
        break;
    }

    case OrientationSensor::BMX055:
    {
        Serial.println("[SensorMgr] Creating BMX055 handler");
        BMX055_Addresses addrs; // ★ БЫЛО BMX055_Handler::I2CAddresses
        addrs.accel = config.imu.bmx055AccelAddress;
        addrs.gyro = config.imu.bmx055GyroAddress;
        addrs.mag = config.imu.bmx055MagAddress;
        _bmx055 = new BMX055_Handler(_wire, addrs);
        if (!_bmx055->begin())
        {
            Serial.println("[SensorMgr] BMX055 init FAILED");
            return false;
        }
        Serial.println("[SensorMgr] Calibrating BMX055...");
        _bmx055->calibrate(1000);
        break;
    }

    case OrientationSensor::NONE:
    default:
        Serial.println("[SensorMgr] ⚠ No IMU selected!");
        return false;
    }

    // Фильтр Маджвика (один для всех IMU)
    _orientationHandler = new Orientation_Handler(config.imu.filterSampleFreqHz);
    _orientationHandler->begin();

    _updatePeriodMs = config.control.sensorPeriodMs;
    _initialized = true;

    Serial.printf("[SensorMgr] Init OK, using %s\n", getCurrentImuName());
    return true;
}

bool SensorManager::startTask(uint8_t coreId, uint8_t priority)
{
    if (!_initialized)
        return false;

    BaseType_t ok = xTaskCreatePinnedToCore(
        sensorTaskEntry, "SensorTask", 4096, this,
        priority, &_taskHandle, coreId);

    return ok == pdPASS;
}

void SensorManager::sensorTaskEntry(void *param)
{
    static_cast<SensorManager *>(param)->sensorTaskLoop();
}

void SensorManager::sensorTaskLoop()
{
    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        readImu();
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(_updatePeriodMs));
    }
}

// ★ Унифицированное чтение: вызывает readData() у нужного Handler'а
void SensorManager::readImu()
{
    if (!_initialized)
        return;

    bool hasNewData = false;

    // Вызываем readData() у того Handler'а, который был создан
    if (_mpu6500)
    {
        hasNewData = _mpu6500->readData();
    }
    else if (_bmx055)
    {
        hasNewData = _bmx055->readData();
    }

    if (hasNewData && _orientationHandler)
    {
        // Данные уже в l_mpuData (записаны Handler'ом)
        _orientationHandler->updateOrientation();

        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(5)) == pdTRUE)
        {
            _data.imu.quat_w = l_mpuData.quatW;
            _data.imu.quat_x = l_mpuData.quatX;
            _data.imu.quat_y = l_mpuData.quatY;
            _data.imu.quat_z = l_mpuData.quatZ;
            _data.imu.roll = l_mpuData.roll;
            _data.imu.pitch = l_mpuData.pitch;
            _data.imu.yaw = l_mpuData.yaw;

            _data.imu.accel_x = (int16_t)l_mpuData.accelX;
            _data.imu.accel_y = (int16_t)l_mpuData.accelY;
            _data.imu.accel_z = (int16_t)l_mpuData.accelZ;
            _data.imu.gyro_x = (int16_t)l_mpuData.gyroX;
            _data.imu.gyro_y = (int16_t)l_mpuData.gyroY;
            _data.imu.gyro_z = (int16_t)l_mpuData.gyroZ;
            _data.imu.mag_x = (int16_t)l_mpuData.magX;
            _data.imu.mag_y = (int16_t)l_mpuData.magY;
            _data.imu.mag_z = (int16_t)l_mpuData.magZ;
            _data.imu.temperature = l_mpuData.temperature;

            _data.lastUpdateMs = millis();
            xSemaphoreGive(_mutex);
        }
    }
}

// Геттеры (без изменений)
bool SensorManager::getImuData(IMU_Data &out) const
{
    if (!_initialized)
        return false;
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        out = _data.imu;
        xSemaphoreGive(_mutex);
        return true;
    }
    return false;
}

bool SensorManager::getSensorData(SensorDataBlock &out) const
{
    if (!_initialized)
        return false;
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        out = _data;
        xSemaphoreGive(_mutex);
        return true;
    }
    return false;
}

float SensorManager::getQuatW() const { return _data.imu.quat_w; }
float SensorManager::getQuatX() const { return _data.imu.quat_x; }
float SensorManager::getQuatY() const { return _data.imu.quat_y; }
float SensorManager::getQuatZ() const { return _data.imu.quat_z; }
float SensorManager::getRoll() const { return _data.imu.roll; }
float SensorManager::getPitch() const { return _data.imu.pitch; }
float SensorManager::getYaw() const { return _data.imu.yaw; }
int16_t SensorManager::getAccelX() const { return _data.imu.accel_x; }
int16_t SensorManager::getAccelY() const { return _data.imu.accel_y; }
int16_t SensorManager::getAccelZ() const { return _data.imu.accel_z; }
int16_t SensorManager::getGyroX() const { return _data.imu.gyro_x; }
int16_t SensorManager::getGyroY() const { return _data.imu.gyro_y; }
int16_t SensorManager::getGyroZ() const { return _data.imu.gyro_z; }
int16_t SensorManager::getMagX() const { return _data.imu.mag_x; }
int16_t SensorManager::getMagY() const { return _data.imu.mag_y; }
int16_t SensorManager::getMagZ() const { return _data.imu.mag_z; }
float SensorManager::getTemperature() const { return _data.imu.temperature; }

bool SensorManager::calibrateImu(uint16_t samples)
{
    if (!_initialized)
        return false;
    if (_mpu6500)
        return _mpu6500->calibrate(samples);
    if (_bmx055)
        return _bmx055->calibrate(samples);
    return false;
}

bool SensorManager::isReady() const { return _initialized; }

const char *SensorManager::getCurrentImuName() const
{
    switch (_currentImuType)
    {
    case OrientationSensor::MPU6500:
        return "MPU6500";
    case OrientationSensor::BMX055:
        return "BMX055";
    default:
        return "NONE";
    }
}