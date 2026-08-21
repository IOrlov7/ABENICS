// File: HMI_Client/Comms/Data/ProtocolEnums.cs
//
// НАЗНАЧЕНИЕ:
// Перечисления команд и флагов состояния.
// Синхронизированы с ESP32 TelemetryPacket.h и форматом из 11.txt.

namespace HMI_Client.Comms.Data
{
    /// <summary>
    /// Идентификаторы команд (ПК → ESP32).
    /// Соответствует enum class CommandId : uint8_t в TelemetryPacket.h
    /// </summary>
    public enum CommandId : byte
    {
        CMD_IDLE      = 0,   // Покой / регистрация клиента
        CMD_JOYSTICK  = 1,   // Управление джойстиком
        CMD_HOME      = 2,   // Домой
        CMD_CALIBRATE = 3,   // Калибровка
        CMD_ESTOP     = 4,   // Аварийный стоп
        CMD_SET_SERVO = 5,   // Установка серво
        CMD_SET_MODE  = 6,   // Режим
        CMD_SET_ANGLE = 7,   // Угол
        CMD_RESUME    = 8,   // Возобновление
        CMD_COUNT            // Количество команд (не отправляется)
    }

    /// <summary>
    /// Флаги состояния системы (битовая маска).
    /// Соответствует enum StatusFlags : uint8_t в TelemetryPacket.h
    /// </summary>
    [System.Flags]
    public enum StatusFlags : byte
    {
        FLAG_ESTOP          = 0x01,
        FLAG_WIFI_CONNECTED = 0x02,
        FLAG_IMU_OK         = 0x04,
        FLAG_STEPPERS_EN    = 0x08,
        FLAG_CALIBRATED     = 0x10,
        FLAG_SPI_ERROR      = 0x20,
        FLAG_I2C_ERROR      = 0x40,
        FLAG_RESERVED       = 0x80
    }
}