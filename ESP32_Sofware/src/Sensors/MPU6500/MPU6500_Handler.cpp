#include "Sensors/MPU6500/MPU6500_Handler.h"

// Инициализация внешних переменных DataBlock (только один раз в проекте)
GlobalDataBlock g_mpuData;
LocalDataBlock l_mpuData;

MPU6500_Handler::MPU6500_Handler(TwoWire& wire, uint8_t addr) 
    : imu(&wire, static_cast<bfs::Mpu6500::I2cAddr>(addr)), i2cBus(&wire), i2cAddress(addr) {
}

bool MPU6500_Handler::begin() {
    i2cBus->begin();
    i2cBus->setClock(400000); // 400 kHz для стабильной высокоскоростной передачи
    
    if (!imu.Begin()) {
        l_mpuData.isError = true;
        return false;
    }
    
    // Оптимальная настройка для баланса между шумом и откликом
    imu.ConfigAccelRange(bfs::Mpu6500::ACCEL_RANGE_4G);
    imu.ConfigGyroRange(bfs::Mpu6500::GYRO_RANGE_500DPS);
    imu.ConfigDlpfBandwidth(bfs::Mpu6500::DLPF_BANDWIDTH_41HZ); // Аппаратный ФНЧ для подавления шума
    
    l_mpuData.isError = false;
    return true;
}

bool MPU6500_Handler::calibrate(uint16_t samples) {
    l_mpuData.isError = false;
    double sumAx = 0, sumAy = 0, sumAz = 0;
    double sumGx = 0, sumGy = 0, sumGz = 0;
    
    delay(100); 
    
    for (uint16_t i = 0; i < samples; i++) {
        if (imu.Read()) {
            sumAx += imu.accel_x_mps2();
            sumAy += imu.accel_y_mps2();
            sumAz += imu.accel_z_mps2();
            sumGx += imu.gyro_x_radps();
            sumGy += imu.gyro_y_radps();
            sumGz += imu.gyro_z_radps();
        }
        delay(2); 
    }
    
    // Гироскоп: в покое скорости = 0. Вычитаем среднее.
    g_mpuData.gyroOffsetX = sumGx / samples;
    g_mpuData.gyroOffsetY = sumGy / samples;
    g_mpuData.gyroOffsetZ = sumGz / samples;
    
    // Акселерометр: По осям X и Y гравитации нет, вычитаем просто среднее (bias).
    g_mpuData.accelOffsetX = sumAx / samples;
    g_mpuData.accelOffsetY = sumAy / samples;
    
    // По оси Z вычитаем bias, но ОСТАВЛЯЕМ 9.81!
    double meanZ = sumAz / samples;
    g_mpuData.accelOffsetZ = meanZ - 9.81; 
    
    // !!! ВОТ ЭТОЙ СТРОКИ НЕ ХВАТАЛО !!!
    g_mpuData.isCalibrated = true; 
    
    g_mpuData.lastCalibrationTime = millis();
    
    return true;
}

// Добавьте эти поля в начало файла (после глобальных переменных)
static float filteredAccelX = 0;
static float filteredAccelY = 0;
static float filteredAccelZ = 9.81f;
static bool firstRead = true;

bool MPU6500_Handler::readData() {
    if (imu.Read()) {
        l_mpuData.isNewData = true;
        l_mpuData.lastReadTime = millis();
        
        // Сырые данные с чипа
        float rawAx = imu.accel_x_mps2();
        float rawAy = imu.accel_y_mps2();
        float rawAz = imu.accel_z_mps2();
        float rawGx = imu.gyro_x_radps();
        float rawGy = imu.gyro_y_radps();
        float rawGz = imu.gyro_z_radps();

        // ═══ МАППИНГ ОСЕЙ (FLU) ═══
        // Судя по вашим логам, меняем местами X/Y и инвертируем Z
        float ax_mapped =  rawAy;   // Физический Y платы -> Вперёд
        float ay_mapped =  rawAx;   // Физический X платы -> Влево
        float az_mapped = -rawAz;   // Инвертируем Z, чтобы "вверх" был +g

        float gx_mapped =  rawGy;
        float gy_mapped =  rawGx;
        float gz_mapped = -rawGz;

        // Low-pass фильтр для акселерометра
        const float alpha = 0.2f;
        if (firstRead) {
            filteredAccelX = ax_mapped;
            filteredAccelY = ay_mapped;
            filteredAccelZ = az_mapped;
            firstRead = false;
        } else {
            filteredAccelX = alpha * ax_mapped + (1 - alpha) * filteredAccelX;
            filteredAccelY = alpha * ay_mapped + (1 - alpha) * filteredAccelY;
            filteredAccelZ = alpha * az_mapped + (1 - alpha) * filteredAccelZ;
        }
        
        l_mpuData.accelX = filteredAccelX;
        l_mpuData.accelY = filteredAccelY;
        l_mpuData.accelZ = filteredAccelZ;
        
        l_mpuData.gyroX = gx_mapped;
        l_mpuData.gyroY = gy_mapped;
        l_mpuData.gyroZ = gz_mapped;
        
        l_mpuData.temperature = imu.die_temp_c();
        l_mpuData.isError = false;
        return true;
    }
    l_mpuData.isNewData = false; l_mpuData.isError = true; return false;
}