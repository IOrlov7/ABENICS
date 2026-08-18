#include "Sensors/MPU6500/MPU6500_Handler.h"
#include "MPU6500.h" // Библиотека bolderflight

MPU6500_Handler::MPU6500_Handler(TwoWire& wire, uint8_t addr)
    : _i2cBus(&wire), _i2cAddress(addr), _imu(nullptr) { // ★ ИНИЦИАЛИЗАЦИЯ ЧЛЕНОВ
}

MPU6500_Handler::~MPU6500_Handler() {
    delete _imu;
}

bool MPU6500_Handler::begin() {
    // ★ ПЕРЕДАЁМ НАШУ ШИНУ В bfs::Mpu6500
    _imu = new bfs::Mpu6500(_i2cBus, static_cast<bfs::Mpu6500::I2cAddr>(_i2cAddress));

    // ★ НЕ ВЫЗЫВАЕМ Wire.begin() снова!
    // bfs::Mpu6500::Begin() теперь будет использовать _i2cBus (т.е. нашу шину)

    if (!_imu->Begin()) {
        Serial.printf("[MPU6500] Init FAILED at 0x%02X\n", _i2cAddress);
        return false;
    }

    Serial.printf("[MPU6500] Init OK at 0x%02X\n", _i2cAddress);
    return true;
}

bool MPU6500_Handler::calibrate(uint16_t samples) {
    if (!_imu) return false;

    Serial.printf("[MPU6500] Calibrating (%d samples)...\n", samples);

    float gx = 0, gy = 0, gz = 0;
    uint16_t count = 0;
    uint32_t start = millis();

    while (count < samples && (millis() - start) < 10000) {
        if (_imu->Read()) {
            gx += _imu->gyro_x_radps();
            gy += _imu->gyro_y_radps();
            gz += _imu->gyro_z_radps();
            count++;
        }
        delay(1);
    }

    if (count == 0) {
        Serial.println("[MPU6500] Calibration FAILED");
        return false;
    }

    Serial.printf("[MPU6500] Calibrated (%d samples)\n", count);
    return true;
}

bool MPU6500_Handler::readData() {
    if (!_imu) return false;

    if (_imu->Read()) {
        // ★ Пишем в глобальную l_mpuData (единый формат для всех IMU)
        l_mpuData.accelX = _imu->accel_x_mps2();
        l_mpuData.accelY = _imu->accel_y_mps2();
        l_mpuData.accelZ = _imu->accel_z_mps2();

        l_mpuData.gyroX = _imu->gyro_x_radps();
        l_mpuData.gyroY = _imu->gyro_y_radps();
        l_mpuData.gyroZ = _imu->gyro_z_radps();

        l_mpuData.temperature = _imu->die_temp_c();

        // MPU6500 без магнитометра
        l_mpuData.magX = 0;
        l_mpuData.magY = 0;
        l_mpuData.magZ = 0;

        l_mpuData.isNewData = true;
        l_mpuData.lastReadTime = millis();

        return true;
    }

    l_mpuData.isNewData = false;
    return false;
}