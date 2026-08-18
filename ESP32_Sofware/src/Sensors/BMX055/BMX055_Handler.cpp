#include "Sensors/BMX055/BMX055_Handler.h"
// #include "BMX055.h"  // ← Раскомментируйте когда добавите библиотеку

BMX055_Handler::BMX055_Handler(TwoWire& wire, const BMX055_Addresses& addrs)
    : _i2cBus(&wire), _addrs(addrs), _imu(nullptr) {
}

BMX055_Handler::~BMX055_Handler() {
    delete _imu;
}

bool BMX055_Handler::begin() {
    // Временная заглушка — пока библиотека BMX055 не подключена
    Serial.println("[BMX055] ⚠ STUB MODE: библиотека не подключена");
    Serial.printf("[BMX055] Expected addresses: Accel=0x%02X, Gyro=0x%02X, Mag=0x%02X\n",
                  _addrs.accel, _addrs.gyro, _addrs.mag);
    
    /*
    _imu = new bfs::Bmx055(_i2cBus);
    if (!_imu->Begin(_addrs.accel, _addrs.gyro, _addrs.mag)) {
        Serial.println("[BMX055] Init FAILED");
        return false;
    }
    Serial.println("[BMX055] Init OK");
    return true;
    */
    
    return true;  // Возвращаем true, чтобы система не падала
}

bool BMX055_Handler::calibrate(uint16_t samples) {
    Serial.printf("[BMX055] Calibrating (%d samples)...\n", samples);
    delay(500);
    Serial.println("[BMX055] Calibration done (stub)");
    return true;
}

// ★ Унифицированный метод: читает данные и пишет в l_mpuData
bool BMX055_Handler::readData() {
    /*
    if (_imu && _imu->Read()) {
        l_mpuData.accelX = _imu->accel_x_mps2();
        l_mpuData.accelY = _imu->accel_y_mps2();
        l_mpuData.accelZ = _imu->accel_z_mps2();
        l_mpuData.gyroX  = _imu->gyro_x_radps();
        l_mpuData.gyroY  = _imu->gyro_y_radps();
        l_mpuData.gyroZ  = _imu->gyro_z_radps();
        l_mpuData.magX   = _imu->mag_x_ut();
        l_mpuData.magY   = _imu->mag_y_ut();
        l_mpuData.magZ   = _imu->mag_z_ut();
        l_mpuData.temperature = _imu->temp_c();
        l_mpuData.isNewData = true;
        l_mpuData.lastReadTime = millis();
        return true;
    }
    */
    
    // Заглушка: генерируем синусоиду для отладки
    static uint32_t counter = 0;
    float t = counter * 0.01f;
    
    l_mpuData.accelX = sinf(t) * 0.5f;
    l_mpuData.accelY = cosf(t) * 0.5f;
    l_mpuData.accelZ = 9.81f;
    l_mpuData.gyroX = sinf(t * 0.5f) * 0.1f;
    l_mpuData.gyroY = cosf(t * 0.5f) * 0.1f;
    l_mpuData.gyroZ = 0;
    l_mpuData.magX = 0;
    l_mpuData.magY = 0;
    l_mpuData.magZ = 0;
    l_mpuData.temperature = 25.0f;
    l_mpuData.isNewData = true;
    l_mpuData.lastReadTime = millis();
    
    counter++;
    return true;
}