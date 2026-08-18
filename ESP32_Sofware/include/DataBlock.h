#ifndef DATABLOCK_H
#define DATABLOCK_H

#include <Arduino.h>

struct GlobalDataBlock {
    bool isCalibrated = false;
    
    float accelOffsetX = 0.0f;
    float accelOffsetY = 0.0f;
    float accelOffsetZ = 0.0f;
    float gyroOffsetX = 0.0f;
    float gyroOffsetY = 0.0f;
    float gyroOffsetZ = 0.0f;
    
    uint32_t lastCalibrationTime = 0;
};

struct LocalDataBlock {
    // Акселерометр (м/с²)
    float accelX = 0.0f, accelY = 0.0f, accelZ = 0.0f;
    // Гироскоп (рад/с)
    float gyroX = 0.0f, gyroY = 0.0f, gyroZ = 0.0f;
    // ★ ДОБАВЛЕНО: Магнитометр (мкТл)
    float magX = 0.0f, magY = 0.0f, magZ = 0.0f;
    // Температура (°C)
    float temperature = 0.0f;
    
    // Ориентация (фильтр Маджвика)
    float quatW = 1.0f, quatX = 0.0f, quatY = 0.0f, quatZ = 0.0f;
    float roll = 0.0f, pitch = 0.0f, yaw = 0.0f;
    
    // Флаги
    bool isNewData = false;
    bool isError = false;
    uint32_t lastReadTime = 0;
};

extern GlobalDataBlock g_mpuData;
extern LocalDataBlock l_mpuData;

#endif // DATABLOCK_H