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
//  ★ ВСЕ ТИПЫ СИНХРОНИЗИРОВАНЫ С C# SensorData.cs
// ============================================================

#pragma pack(push, 1)

// ★ ИЗМЕНЕНО: все данные IMU как float (синхронизация с C#)
struct IMU_Data {
    // Ориентация (кватернион + углы Эйлера)
    float quat_w, quat_x, quat_y, quat_z;   // 16 байт
    float roll, pitch, yaw;                  // 12 байт

    // Сырые данные (ВСЕ КАК float, а не int16_t!)
    float accel_x, accel_y, accel_z;        // 12 байт (м/с²)
    float gyro_x,  gyro_y,  gyro_z;         // 12 байт (рад/с)
    float mag_x,   mag_y,   mag_z;          // 12 байт (0 для MPU6500)

    // ★ УБРАЛИ temperature, чтобы влезть в 103 байта
    // Итого: 16 + 12 + 12 + 12 + 12 = 64 байта
};

// ★ УПРОЩЕНО: только angle (для синхронизации с C#)
struct Motor_Telemetry {
    float angle;          // 4 байта (градусы)
    // Итого: 4 байта
};

// ★ УБРАЛИ uptime_ms для экономии места
struct System_State {
    uint8_t current_cmd;   // 1 байт (CommandId)
    uint8_t status_flags;  // 1 байт (StatusFlags)
    int8_t wifi_rssi;      // 1 байт (dBm)
    // Итого: 3 байта
};

struct TelemetryPacket {
    // Заголовок
    uint8_t header[2];       // 2 байта: 0xAA, 0x55
    uint32_t packet_id;      // ★ ИЗМЕНЕНО: uint32_t (4 байта, вместо uint16_t)
    uint32_t timestamp_ms;   // 4 байта

    // Полезная нагрузка
    IMU_Data imu;                    // 64 байта
    Motor_Telemetry stepper_x;      //  4 байта
    Motor_Telemetry stepper_y;      //  4 байта
    uint16_t servo_angles[8];       // 16 байт (8 серво)
    System_State system;            //  3 байта

    // Контрольная сумма
    uint16_t crc16;                  //  2 байта
    // Итого: 2+4+4+64+4+4+16+3+2 = 103 байта ✓
};

#pragma pack(pop)

// Статическая проверка размера
static_assert(sizeof(TelemetryPacket) == 103,
              "TelemetryPacket size mismatch! Check alignment.");

#define TELEMETRY_PACKET_SIZE sizeof(TelemetryPacket)
#define TELEMETRY_HEADER_BYTE_0 0xAA
#define TELEMETRY_HEADER_BYTE_1 0x55