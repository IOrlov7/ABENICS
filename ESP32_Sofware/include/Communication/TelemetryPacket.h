#pragma once
#include <stdint.h>

// ============================================================
//  Перечисления команд и флагов состояния
// ============================================================

enum class CommandId : uint8_t {
    CMD_IDLE        = 0,
    CMD_JOYSTICK    = 1,
    CMD_HOME        = 2,
    CMD_CALIBRATE   = 3,
    CMD_ESTOP       = 4,
    CMD_SET_SERVO   = 5,
    CMD_SET_MODE    = 6,
    CMD_SET_ANGLE   = 7,
    CMD_RESUME      = 8,
    CMD_COUNT
};

enum StatusFlags : uint8_t {
    FLAG_ESTOP          = 0x01,
    FLAG_WIFI_CONNECTED = 0x02,
    FLAG_IMU_OK         = 0x04,
    FLAG_STEPPERS_EN    = 0x08,
    FLAG_CALIBRATED     = 0x10,
    FLAG_SPI_ERROR      = 0x20,
    FLAG_I2C_ERROR      = 0x40,
    FLAG_RESERVED       = 0x80
};

// ============================================================
//  Бинарные структуры (pack = 1, без выравнивания)
// ============================================================

#pragma pack(push, 1)

struct IMU_Data {
    // Ориентация (обработанные данные)
    float quat_w, quat_x, quat_y, quat_z;   // 16 байт
    float roll, pitch, yaw;                  // 12 байт

    // Сырые данные (для калибровки и графиков)
    int16_t accel_x, accel_y, accel_z;       //  6 байт
    int16_t gyro_x,  gyro_y,  gyro_z;        //  6 байт
    int16_t mag_x,   mag_y,   mag_z;         //  6 байт

    float temperature;                       //  4 байта
    // Итого: 50 байт
};

struct Motor_Telemetry {
    float angle;          // Текущее положение вала (градусы)
    float speed;          // Скорость (градусы/сек)
    int8_t direction;     // 1: вперёд, -1: назад, 0: стоп
    uint8_t state;        // 0: выкл, 1: работа, 2: ошибка, 3: удержание
    // Итого: 10 байт
};

struct System_State {
    uint8_t current_cmd;   // CommandId (текущая выполняемая команда)
    uint8_t status_flags;  // Битовая маска StatusFlags
    int8_t wifi_rssi;      // Уровень сигнала Wi-Fi (dBm)
    uint32_t uptime_ms;    // millis()
    // Итого: 7 байт
};

struct TelemetryPacket {
    // Заголовок
    uint8_t header[2];       // 0xAA, 0x55
    uint16_t packet_id;      // Инкрементируемый ID (детект потерь)
    uint32_t timestamp_ms;   // Время отправки (синхронизация с видео)

    // Полезная нагрузка
    IMU_Data imu;                    // 50 байт
    Motor_Telemetry stepper_x;      // 10 байт
    Motor_Telemetry stepper_y;      // 10 байт
    uint16_t servo_angles[8];       // 16 байт (8 серво TD-7120MG)
    System_State system;            //  7 байт

    // Контрольная сумма
    uint16_t crc16;                  //  2 байта
    // Итого: 2+2+4+50+10+10+16+7+2 = 103 байта
};

#pragma pack(pop)

// Статическая проверка размера (защита от случайного изменения)
static_assert(sizeof(TelemetryPacket) == 103,
              "TelemetryPacket size mismatch! Check alignment.");

#define TELEMETRY_PACKET_SIZE sizeof(TelemetryPacket)
#define TELEMETRY_HEADER_BYTE_0 0xAA
#define TELEMETRY_HEADER_BYTE_1 0x55