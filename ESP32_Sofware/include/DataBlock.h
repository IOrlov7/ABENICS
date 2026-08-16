#ifndef DATABLOCK_H
#define DATABLOCK_H

#include <Arduino.h>

// Глобальный блок данных (параметры, сохраняемые между циклами)
struct GlobalDataBlock {
    bool isCalibrated = false;
    
    // Смещения (offsets) для калибровки
    float accelOffsetX = 0.0f;
    float accelOffsetY = 0.0f;
    float accelOffsetZ = 0.0f;
    float gyroOffsetX = 0.0f;
    float gyroOffsetY = 0.0f;
    float gyroOffsetZ = 0.0f;
    
    // Время последней калибровки (вернули это поле!)
    uint32_t lastCalibrationTime = 0;
};

// Локальный блок данных (текущие значения)
struct LocalDataBlock {
    // Данные с датчика
    float accelX = 0.0f, accelY = 0.0f, accelZ = 0.0f; // м/с²
    float gyroX = 0.0f, gyroY = 0.0f, gyroZ = 0.0f;    // рад/с
    float temperature = 0.0f; // °C
    
    // Результаты фильтрации (Ориентация)
    float quatW = 1.0f, quatX = 0.0f, quatY = 0.0f, quatZ = 0.0f; // Кватернион
    float roll = 0.0f, pitch = 0.0f, yaw = 0.0f;                  // Углы Эйлера (градусы)
    
    // Флаги состояния
    bool isNewData = false;
    bool isError = false;
    uint32_t lastReadTime = 0;
};

// Внешние ссылки для доступа из любого файла проекта
extern GlobalDataBlock g_mpuData;
extern LocalDataBlock l_mpuData;

#endif // DATABLOCK_H