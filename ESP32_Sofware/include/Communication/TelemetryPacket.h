// ESP32_Software/include/Communication/TelemetryPacket.h
#pragma once
#include <Arduino.h>

#pragma pack(push, 1) // Отключаем выравнивание для экономии места и точного совпадения с C#
struct TelemetryPacket {
    // 1. Заголовок (для синхронизации потока на C# стороне)
    uint8_t header[4] = {'T', 'E', 'L', 'M'}; 
    uint32_t packetId = 0;
    uint32_t timestamp = 0; // micros() или millis()

    // 2. IMU (BMX055 / MPU6500)
    float imu_temp;
    float accelX, accelY, accelZ;
    float quatW, quatX, quatY, quatZ;
    float roll, pitch, yaw;

    // 3. Кинематика и Моторы (NEMA23 + MT6816)
    float motorX_angle;   // Текущий угол с энкодера
    float motorY_angle;
    uint8_t motorX_state; // 0=OK, 1=Error, 2=Homing
    uint8_t motorY_state;

    // 4. Сервоприводы (PCA9685)
    float servoAngles[8]; // Углы 8 сервоприводов TD-7120MG

    // 5. Геймпад (YWRobot)
    float joy_axes[4];    // X1, Y1, X2, Y2 (от -1.0 до 1.0)
    uint16_t joy_buttons; // Битовая маска нажатых кнопок
    uint8_t control_mode; // Текущий режим работы манипулятора

    // 6. Системная диагностика
    int8_t wifi_rssi;     // Уровень сигнала
    uint8_t system_flags; // Биты ошибок (например, перегрев, I2C fail)
    uint16_t checksum;    // CRC16 для проверки целостности
};
#pragma pack(pop)