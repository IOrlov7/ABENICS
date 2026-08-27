// File: HMI_Client/Comms/Data/SensorData.cs
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace HMI_Client.Comms.Data
{
    public class SensorData : INotifyPropertyChanged
    {
        private TelemetryPacket _packet;

        public void UpdateFromPacket(TelemetryPacket packet)
        {
            _packet = packet;
            OnPropertyChanged(nameof(PacketId));
            OnPropertyChanged(nameof(TimestampMs));
            OnPropertyChanged(nameof(Roll));
            OnPropertyChanged(nameof(Pitch));
            OnPropertyChanged(nameof(Yaw));
            OnPropertyChanged(nameof(AccelX));
            OnPropertyChanged(nameof(AccelY));
            OnPropertyChanged(nameof(AccelZ));
            OnPropertyChanged(nameof(GyroX));
            OnPropertyChanged(nameof(GyroY));
            OnPropertyChanged(nameof(GyroZ));
            OnPropertyChanged(nameof(WifiRssi));
            OnPropertyChanged(nameof(CurrentCmd));
            OnPropertyChanged(nameof(StatusFlags));
            // ★ НОВОЕ: Уведомления для PID-тюнинга
            OnPropertyChanged(nameof(StepperX_Angle));
            OnPropertyChanged(nameof(StepperY_Angle));
            OnPropertyChanged(nameof(TargetAngleX));
            OnPropertyChanged(nameof(TargetAngleY));
            OnPropertyChanged(nameof(TargetSpeedX));
            OnPropertyChanged(nameof(TargetSpeedY));
        }

        public uint PacketId => _packet.PacketId;
        public uint TimestampMs => _packet.TimestampMs;
        public float Roll => _packet.EulerRoll;
        public float Pitch => _packet.EulerPitch;
        public float Yaw => _packet.EulerYaw;
        public float AccelX => _packet.AccelX;
        public float AccelY => _packet.AccelY;
        public float AccelZ => _packet.AccelZ;
        public float GyroX => _packet.GyroX;
        public float GyroY => _packet.GyroY;
        public float GyroZ => _packet.GyroZ;
        public sbyte WifiRssi => _packet.WifiRssi;
        public CommandId CurrentCmd => (CommandId)_packet.CurrentCmd;
        public StatusFlags StatusFlags => (StatusFlags)_packet.StatusFlags;

        // ★ НОВОЕ: Свойства для PID-тюнинга
        public float StepperX_Angle => _packet.StepperX_Angle;
        public float StepperY_Angle => _packet.StepperY_Angle;
        public float TargetAngleX => _packet.TargetAngleX;
        public float TargetAngleY => _packet.TargetAngleY;
        public float TargetSpeedX => _packet.TargetSpeedX;
        public float TargetSpeedY => _packet.TargetSpeedY;

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}