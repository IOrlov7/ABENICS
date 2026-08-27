using System;
using System.Runtime.InteropServices;
using HMI_Client.Utils;

namespace HMI_Client.Comms.Data
{
    /// <summary>
    /// Бинарная структура пакета телеметрии (119 байт, Pack=1).
    /// Синхронизирована с ESP32 TelemetryPacket.h.
    /// Все поля little-endian, все IMU-данные как float.
    /// </summary>
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct TelemetryPacket
    {
        public const int ExpectedSize = 119; // ★ ОБНОВЛЕНО: было 103, стало 119

        // Offset 0-1: Заголовок
        public byte Header0;          // 0: 0xAA
        public byte Header1;          // 1: 0x55

        // Offset 2-9: Идентификация и время
        public uint PacketId;         // 2: счётчик (uint32 LE)
        public uint TimestampMs;      // 6: millis()

        // Offset 10-73: IMU (все float, 16 полей * 4 = 64 байта)
        public float QuatW;           // 10
        public float QuatX;           // 14
        public float QuatY;           // 18
        public float QuatZ;           // 22
        public float EulerRoll;       // 26
        public float EulerPitch;      // 30
        public float EulerYaw;        // 34
        public float AccelX;          // 38
        public float AccelY;          // 42
        public float AccelZ;          // 46
        public float GyroX;           // 50
        public float GyroY;           // 54
        public float GyroZ;           // 58
        public float MagX;            // 62
        public float MagY;            // 66
        public float MagZ;            // 70

        // Offset 74-81: Шаговые моторы (текущее положение)
        public float StepperX_Angle;  // 74
        public float StepperY_Angle;  // 78

        // Offset 82-97: Сервоприводы (uint16[8] = 16 байт)
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public ushort[] ServoAngles;

        // ========================================================
        // ★ НОВЫЕ ПОЛЯ для PID-тюнинга и визуализации каскада (16 байт)
        // ========================================================
        public float TargetAngleX;    // 98: Целевой угол для внутреннего контура X
        public float TargetAngleY;    // 102: Целевой угол для внутреннего контура Y
        public float TargetSpeedX;    // 106: Выход PID (шаги/с или ШИМ) для X
        public float TargetSpeedY;    // 110: Выход PID (шаги/с или ШИМ) для Y
        // ========================================================

        // Offset 114-116: Система (3 байта)
        public byte CurrentCmd;       // 114
        public byte StatusFlags;      // 115
        public sbyte WifiRssi;        // 116

        // Offset 117-118: CRC16 (2 байта)
        public ushort Crc16;          // 117

        /// <summary>
        /// Проверка заголовка пакета
        /// </summary>
        public bool IsHeaderValid => Header0 == 0xAA && Header1 == 0x55;

        /// <summary>
        /// Десериализация из массива байт
        /// </summary>
        public static TelemetryPacket FromBytes(byte[] data)
        {
            if (data == null || data.Length < ExpectedSize)
                throw new ArgumentException($"Expected {ExpectedSize} bytes, got {data?.Length ?? 0}");

            int size = Marshal.SizeOf<TelemetryPacket>();
            IntPtr ptr = Marshal.AllocHGlobal(size);
            try
            {
                Marshal.Copy(data, 0, ptr, Math.Min(size, data.Length));
                return Marshal.PtrToStructure<TelemetryPacket>(ptr);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }

        /// <summary>
        /// Сериализация в массив байт
        /// </summary>
        public byte[] ToBytes()
        {
            int size = Marshal.SizeOf<TelemetryPacket>();
            byte[] bytes = new byte[size];
            IntPtr ptr = Marshal.AllocHGlobal(size);
            try
            {
                Marshal.StructureToPtr(this, ptr, false);
                Marshal.Copy(ptr, bytes, 0, size);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
            return bytes;
        }
    }
}