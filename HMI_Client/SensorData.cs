using System.Runtime.InteropServices;

namespace HMI_Client
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct TelemetryPacket
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public byte[] Header;
        public uint PacketId;
        public uint Timestamp;

        // IMU
        public float ImuTemp;
        public float AccelX, AccelY, AccelZ;
        public float QuatW, QuatX, QuatY, QuatZ;
        public float Roll, Pitch, Yaw;

        // Моторы
        public float MotorXAngle;
        public float MotorYAngle;
        public byte MotorXState;
        public byte MotorYState;

        // Серво
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public float[] ServoAngles;

        // Геймпад
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public float[] JoyAxes;
        public ushort JoyButtons;
        public byte ControlMode;

        // Система
        public sbyte WifiRssi;
        public byte SystemFlags;
        public ushort Checksum;
    }
}