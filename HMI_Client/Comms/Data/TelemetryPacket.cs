using System;
using System.Runtime.InteropServices;
using HMI_Client.Utils;

namespace HMI_Client.Comms.Data
{
    /// <summary>
    /// Бинарная структура пакета телеметрии (103 байта, Pack=1).
    /// Синхронизирована с ESP32 TelemetryPacket.h и форматом из 11.txt.
    /// Все поля little-endian, все IMU-данные как float.
    /// </summary>
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct TelemetryPacket
    {
        public const int ExpectedSize = 103;

        // Offset 0-1: Заголовок
        public byte Header0;          // 0xAA
        public byte Header1;          // 0x55

        // Offset 2-5: Счётчик пакетов (uint32 LE)
        public uint PacketId;

        // Offset 6-9: Timestamp (uint32 LE)
        public uint TimestampMs;

        // Offset 10-73: IMU (все float)
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

        // Offset 74-81: Шаговые моторы
        public float StepperX_Angle;  // 74
        public float StepperY_Angle;  // 78

        // Offset 82-97: Сервоприводы (uint16[8] = 16 байт)
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public ushort[] ServoAngles;

        // Offset 98-100: Система
        public byte CurrentCmd;       // 98
        public byte StatusFlags;      // 99
        public sbyte WifiRssi;        // 100

        // Offset 101-102: CRC16
        public ushort Crc16;          // 101

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