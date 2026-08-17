#include "Sensors/SensorManager.h"
#include "Sensors/MPU6500/MPU6500_Handler.h"
#include "Sensors/MPU6500/MPU6500_Orientation_Handler.h"
#include "DataBlock.h"   // l_mpuData

// ============================================================
//  Конструктор / деструктор
// ============================================================

SensorManager::SensorManager()
    : _wire(0)
    , _imuHandler(nullptr)
    , _orientationHandler(nullptr)
    , _mutex(nullptr)
    , _taskHandle(nullptr)
    , _updatePeriodMs(10)   // 100 Гц по умолчанию
    , _initialized(false)
{
    memset(&_data, 0, sizeof(SensorDataBlock));
    _mutex = xSemaphoreCreateMutex();
}

SensorManager::~SensorManager() {
    if (_taskHandle) vTaskDelete(_taskHandle);
    if (_mutex) vSemaphoreDelete(_mutex);
    delete _imuHandler;
    delete _orientationHandler;
}

SensorManager& SensorManager::instance() {
    static SensorManager inst;
    return inst;
}

// ============================================================
//  Инициализация
// ============================================================

bool SensorManager::begin(const ProjectConfig& config) {
    // Создаём обработчики IMU
    _imuHandler = new MPU6500_Handler(_wire, 0x68);
    _orientationHandler = new Orientation_Handler(100.0f);

    if (!_imuHandler->begin()) {
        Serial.println("[SensorMgr] MPU6500 init FAILED");
        return false;
    }

    Serial.println("[SensorMgr] Calibrating MPU6500...");
    _imuHandler->calibrate(1000);
    _orientationHandler->begin();

    _updatePeriodMs = 10;  // 100 Гц
    _initialized = true;
    Serial.println("[SensorMgr] Init OK");
    return true;
}

// ============================================================
//  Запуск задачи опроса датчиков
// ============================================================

bool SensorManager::startTask(uint8_t coreId, uint8_t priority) {
    if (!_initialized) return false;

    BaseType_t ok = xTaskCreatePinnedToCore(
        sensorTaskEntry,
        "SensorTask",
        4096,
        this,
        priority,
        &_taskHandle,
        coreId
    );

    if (ok != pdPASS) {
        Serial.println("[SensorMgr] Task creation FAILED");
        return false;
    }
    Serial.println("[SensorMgr] Task started");
    return true;
}

void SensorManager::sensorTaskEntry(void* param) {
    static_cast<SensorManager*>(param)->sensorTaskLoop();
}

void SensorManager::sensorTaskLoop() {
    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;) {
        // 1. IMU
        readImu();

        // 2. Энкодеры MT6816 (Этап 6)
        // readEncoders();

        // 3. Будущие датчики...

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(_updatePeriodMs));
    }
}

// ============================================================
//  Чтение IMU
// ============================================================

void SensorManager::readImu() {
    if (!_initialized || !_imuHandler) return;

    if (_imuHandler->readData()) {
        _orientationHandler->updateOrientation();

        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            // Кватернион
            _data.imu.quat_w = l_mpuData.quatW;
            _data.imu.quat_x = l_mpuData.quatX;
            _data.imu.quat_y = l_mpuData.quatY;
            _data.imu.quat_z = l_mpuData.quatZ;

            // Углы Эйлера
            _data.imu.roll  = l_mpuData.roll;
            _data.imu.pitch = l_mpuData.pitch;
            _data.imu.yaw   = l_mpuData.yaw;

            // Сырые данные
            _data.imu.accel_x = (int16_t)l_mpuData.accelX;
            _data.imu.accel_y = (int16_t)l_mpuData.accelY;
            _data.imu.accel_z = (int16_t)l_mpuData.accelZ;

            _data.imu.gyro_x = (int16_t)l_mpuData.gyroX;
            _data.imu.gyro_y = (int16_t)l_mpuData.gyroY;
            _data.imu.gyro_z = (int16_t)l_mpuData.gyroZ;

            _data.imu.temperature = l_mpuData.temperature;

            // MPU6500 без магнитометра
            _data.imu.mag_x = 0;
            _data.imu.mag_y = 0;
            _data.imu.mag_z = 0;

            _data.lastUpdateMs = millis();

            xSemaphoreGive(_mutex);
        }
    }
}

// ============================================================
//  Чтение энкодеров MT6816 (Этап 6 — заглушка)
// ============================================================

void SensorManager::readEncoders() {
    // TODO: SPI чтение MT6816
    // _data.encoderX.angleDeg = ...;
    // _data.encoderY.angleDeg = ...;
}

// ============================================================
//  Потокобезопасные геттеры
// ============================================================

bool SensorManager::getImuData(IMU_Data& out) const {
    if (!_initialized) return false;
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        out = _data.imu;
        xSemaphoreGive(_mutex);
        return true;
    }
    return false;
}

bool SensorManager::getSensorData(SensorDataBlock& out) const {
    if (!_initialized) return false;
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        out = _data;
        xSemaphoreGive(_mutex);
        return true;
    }
    return false;
}

float   SensorManager::getQuatW()  const { return _data.imu.quat_w; }
float   SensorManager::getQuatX()  const { return _data.imu.quat_x; }
float   SensorManager::getQuatY()  const { return _data.imu.quat_y; }
float   SensorManager::getQuatZ()  const { return _data.imu.quat_z; }
float   SensorManager::getRoll()   const { return _data.imu.roll; }
float   SensorManager::getPitch()  const { return _data.imu.pitch; }
float   SensorManager::getYaw()    const { return _data.imu.yaw; }
int16_t SensorManager::getAccelX() const { return _data.imu.accel_x; }
int16_t SensorManager::getAccelY() const { return _data.imu.accel_y; }
int16_t SensorManager::getAccelZ() const { return _data.imu.accel_z; }
int16_t SensorManager::getGyroX()  const { return _data.imu.gyro_x; }
int16_t SensorManager::getGyroY()  const { return _data.imu.gyro_y; }
int16_t SensorManager::getGyroZ()  const { return _data.imu.gyro_z; }
int16_t SensorManager::getMagX()   const { return 0; }
int16_t SensorManager::getMagY()   const { return 0; }
int16_t SensorManager::getMagZ()   const { return 0; }
float   SensorManager::getTemperature() const { return _data.imu.temperature; }

// ============================================================
//  Калибровка
// ============================================================

bool SensorManager::calibrateImu(uint16_t samples) {
    if (!_initialized || !_imuHandler) return false;
    return _imuHandler->calibrate(samples);
}

bool SensorManager::isReady() const {
    return _initialized;
}