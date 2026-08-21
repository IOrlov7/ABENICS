// File: HMI_Client/Comms/Data/SensorData.cs
//
// НАЗНАЧЕНИЕ:
// Этот класс представляет собой удобную для привязки к интерфейсу (UI binding) версию данных телеметрии.
// Он преобразует сырые данные из TelemetryPacket в свойства, которые могут отслеживаться WPF.
// Он НЕ содержит логики обработки данных или сетевых операций.
//
// ОТВЕЧАЕТ ЗА:
// - Предоставление данных из TelemetryPacket через свойства WPF.
// - Уведомление WPF об изменениях данных (INotifyPropertyChanged).
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
        
        // ★ ИСПРАВЛЕНО: packet → _packet
        public CommandId CurrentCmd => (CommandId)_packet.CurrentCmd;
        public StatusFlags StatusFlags => (StatusFlags)_packet.StatusFlags;

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}