using System;
using System.Runtime.InteropServices;

namespace HMI_Client
{
    public enum CommandId : byte
    {
        CMD_IDLE = 0,
        CMD_JOYSTICK = 1,
        CMD_HOME = 2,
        CMD_CALIBRATE = 3,
        CMD_ESTOP = 4,
        CMD_SET_SERVO = 5,
        CMD_SET_MODE = 6,
        CMD_SET_ANGLE = 7,
        CMD_RESUME = 8
    }

    [Flags]
    public enum StatusFlags : byte
    {
        FLAG_ESTOP = 0x01,
        FLAG_WIFI_CONNECTED = 0x02,
        FLAG_IMU_OK = 0x04,
        FLAG_STEPPERS_EN = 0x08,
        FLAG_CALIBRATED = 0x10,
        FLAG_SPI_ERROR = 0x20,
        FLAG_I2C_ERROR = 0x40
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct TelemetryPacket
    {
        // Header (2 bytes)
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
        public byte[] Header;

        // Meta (8 bytes) — ★ ИЗМЕНЕНО: packet_id теперь uint (4 байта)
        public uint PacketId;
        public uint TimestampMs;

        // IMU Data (64 bytes) — ★ ВСЕ float
        public float QuatW, QuatX, QuatY, QuatZ;
        public float EulerRoll, EulerPitch, EulerYaw;
        public float AccelX, AccelY, AccelZ;
        public float GyroX, GyroY, GyroZ;
        public float MagX, MagY, MagZ;

        // Motors (8 bytes) — ★ УПРОЩЕНО: только angle
        public float StepperX_Angle;
        public float StepperY_Angle;

        // Servos (16 bytes)
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public ushort[] ServoAngles;

        // System State (3 bytes)
        public StatusFlags StatusFlags;
        public sbyte WifiRssi;
        public CommandId CurrentCmd;

        // CRC16 (2 bytes)
        public ushort Crc16;

        public const int ExpectedSize = 103;
    }

    public static class PacketValidator
    {
        public static bool Validate(byte[] data)
        {
            if (data == null || data.Length != TelemetryPacket.ExpectedSize)
                return false;

            if (data[0] != 0xAA || data[1] != 0x55)
                return false;

            ushort calculatedCrc = CalculateCrc16(data, data.Length - 2);
            ushort receivedCrc = (ushort)(data[101] | (data[102] << 8));

            return calculatedCrc == receivedCrc;
        }

        public static ushort CalculateCrc16(byte[] data, int length)
        {
            ushort crc = 0xFFFF;
            for (int pos = 0; pos < length; pos++)
            {
                crc ^= data[pos];
                for (int i = 0; i < 8; i++)
                {
                    if ((crc & 1) != 0)
                    {
                        crc >>= 1;
                        crc ^= 0x8408;
                    }
                    else
                    {
                        crc >>= 1;
                    }
                }
            }
            return crc;
        }

        public static byte[] BuildCommandPacket(CommandId cmdId, byte[]? payload = null)
        {
            int payloadLen = payload?.Length ?? 0;
            int totalLen = 2 + 1 + payloadLen + 2;
            byte[] packet = new byte[totalLen];

            packet[0] = 0xAA;
            packet[1] = 0x55;
            packet[2] = (byte)cmdId;

            if (payload != null && payloadLen > 0)
            {
                Array.Copy(payload, 0, packet, 3, payloadLen);
            }

            ushort crc = CalculateCrc16(packet, totalLen - 2);
            packet[totalLen - 2] = (byte)(crc & 0xFF);
            packet[totalLen - 1] = (byte)((crc >> 8) & 0xFF);

            return packet;
        }
    }
}