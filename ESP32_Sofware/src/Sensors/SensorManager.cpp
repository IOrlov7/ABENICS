#include "Sensors/SensorManager.h"

SensorManager::SensorManager()
    : _bmx055(nullptr),
      _mpu6500(nullptr),
      _mpu6500Address(0x68),
      _ready(false),
      _mutex(nullptr) {
}

SensorManager::~SensorManager() {
    delete _bmx055;
    delete _mpu6500;
}

SensorManager& SensorManager::instance() {
    static SensorManager manager;
    return manager;
}

void SensorManager::lock() {
    if (_mutex != nullptr) {
        xSemaphoreTake(_mutex, portMAX_DELAY);
    }
}

void SensorManager::unlock() {
    if (_mutex != nullptr) {
        xSemaphoreGive(_mutex);
    }
}

bool SensorManager::begin(const ProjectConfig& cfg) {
    _cfg = cfg;

    if (_mutex == nullptr) {
        _mutex = xSemaphoreCreateMutex();
    }

    if (_mutex == nullptr) {
        return false;
    }

    if (_cfg.orientationSensor == OrientationSensor::BMX055) {
        if (_bmx055 == nullptr) {
            _bmx055 = new BMX055_Handler();
        }

        _ready = _bmx055->begin();
    } else {
        if (_mpu6500 == nullptr) {
            // Важно: у тебя MPU6500_Handler требует TwoWire и адрес.
            // Если AD0 у MPU6500 подтянут к 3.3V, адрес может быть 0x69.
            _mpu6500 = new MPU6500_Handler(Wire, _mpu6500Address);
        }

        _ready = _mpu6500->begin();
    }

    return _ready;
}

bool SensorManager::update() {
    if (!_ready) {
        return false;
    }

    ImuData temp;
    bool ok = false;

    if (_cfg.orientationSensor == OrientationSensor::BMX055) {
        ok = readBmx(temp);
    } else {
        ok = readMpu(temp);
    }

    if (ok) {
        lock();
        _data = temp;
        unlock();
    }

    return ok;
}

bool SensorManager::getImu(ImuData& out) {
    if (!_ready) {
        return false;
    }

    lock();
    out = _data;
    unlock();

    return true;
}

bool SensorManager::readBmx(ImuData& out) {
    if (_bmx055 == nullptr) {
        return false;
    }

    BMX055Data raw;

    if (!_bmx055->read(raw)) {
        return false;
    }

    out.timestampMs = raw.timestampMs;

    out.accelOk = raw.accelOk;
    out.gyroOk = raw.gyroOk;
    out.magOk = raw.magOk;

    for (int i = 0; i < 3; ++i) {
        out.accelG[i] = raw.accelG[i];
        out.gyroDps[i] = raw.gyroDps[i];
        out.magRaw[i] = static_cast<float>(raw.magRaw[i]);
    }

    out.source = OrientationSensor::BMX055;

    return true;
}

bool SensorManager::readMpu(ImuData& out) {
    if (_mpu6500 == nullptr) {
        return false;
    }

    // =========================================================
    // TODO: адаптировать под реальный API твоего MPU6500_Handler.
    // =========================================================
    //
    // Желательно привести MPU6500_Handler к тому же виду, что и BMX055_Handler:
    //
    // bool begin();
    // bool read(SomeMpuData& data);
    //
    // Затем здесь:
    //
    // SomeMpuData raw;
    //
    // if (!_mpu6500->read(raw)) {
    //     return false;
    // }
    //
    // out.timestampMs = raw.timestampMs;
    // out.accelOk = raw.accelOk;
    // out.gyroOk = raw.gyroOk;
    // out.magOk = false;
    //
    // for (int i = 0; i < 3; ++i) {
    //     out.accelG[i] = raw.accelG[i];
    //     out.gyroDps[i] = raw.gyroDps[i];
    // }
    //
    // out.source = OrientationSensor::MPU6500;
    // return true;
    //
    // Если MPU6500 отдаёт raw-значения, типовые коэффициенты:
    // accel ±2g: 16384 LSB/g
    // gyro ±250: 131 LSB/dps
    //
    // accelG = rawAccel / 16384.0f;
    // gyroDps = rawGyro / 131.0f;
    // =========================================================

    return false;
}