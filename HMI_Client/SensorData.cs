// SensorData.cs
namespace IMU_Visualizer
{
    public struct SensorData
    {
        public float Temperature;
        public float AccX, AccY, AccZ;
        public float QuatW, QuatX, QuatY, QuatZ;
        public float Roll, Pitch, Yaw;

        public override string ToString() =>
            $"T:{Temperature:F1}  " +
            $"A:[{AccX:F2},{AccY:F2},{AccZ:F2}]  " +
            $"Q:[{QuatW:F3},{QuatX:F3},{QuatY:F3},{QuatZ:F3}]  " +
            $"RPY:[{Roll:F1},{Pitch:F1},{Yaw:F1}]";
    }
}