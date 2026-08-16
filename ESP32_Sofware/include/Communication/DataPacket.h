#pragma once
#include <Arduino.h>

struct DataPacket {
    float temperature = 0.0f;
    float accelX = 0.0f, accelY = 0.0f, accelZ = 0.0f;
    float quatW = 0.0f, quatX = 0.0f, quatY = 0.0f, quatZ = 0.0f;
    float roll = 0.0f, pitch = 0.0f, yaw = 0.0f;

    // Форматирует данные ровно в тот вид, который ждет ваш C# парсер
    String toString() const {
        char buffer[200];
        snprintf(buffer, sizeof(buffer),
            "DATA|T:%.1f|AX:%.2f|AY:%.2f|AZ:%.2f|QW:%.2f|QX:%.2f|QY:%.2f|QZ:%.2f|R:%.1f|P:%.1f|Y:%.1f",
            temperature, accelX, accelY, accelZ,
            quatW, quatX, quatY, quatZ, roll, pitch, yaw);
        return String(buffer);
    }
};