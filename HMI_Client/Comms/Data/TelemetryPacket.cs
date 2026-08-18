// File: Comms/Data/TelemetryPacket.cs
using System;
using System.Runtime.InteropServices;

namespace HMI_Client.Comms.Data
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
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
        public byte[] Header;

        public uint PacketId;
        public uint TimestampMs;

        public float QuatW, QuatX, QuatY, QuatZ;
        public float EulerRoll, EulerPitch, EulerYaw;
        public float AccelX, AccelY, AccelZ;
        public float GyroX, GyroY, GyroZ;
        public float MagX, MagY, MagZ;

        public float StepperX_Angle;
        public float StepperY_Angle;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public ushort[] ServoAngles;

        public StatusFlags StatusFlags;
        public sbyte WifiRssi;
        public CommandId CurrentCmd;

        public ushort Crc16;

        public const int ExpectedSize = 103;
    }
}