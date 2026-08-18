// File: HMI_Client/Comms/SerialSender.cs
//
// НАЗНАЧЕНИЕ:
// Реализация интерфейса ICommInterface для получения данных и отправки команд через COM-порт.
// Предоставляет альтернативный или основной канал связи, когда Wi-Fi недоступен или ненадежен.
//
// ОТВЕЧАЕТ ЗА:
// - Подключение к ESP32 через COM-порт (System.IO.Ports.SerialPort).
// - Чтение бинарных пакетов телеметрии из порта.
// - Проверку CRC и валидацию пакетов.
// - Отправку бинарных команд через COM-порт.
// - Управление потоком данных (BackgroundWorker или Task с BlockingCollection для потокобезопасности).

using System;
using System.IO.Ports;
using System.Threading;
using System.Threading.Tasks;
using HMI_Client.Comms.Data;
using HMI_Client.Utils;

namespace HMI_Client.Comms
{
    public class SerialSender : ICommInterface
    {
        private SerialPort? _serialPort;
        private readonly object _lock = new object();
        private bool _isRunning;
        private CancellationTokenSource? _cancellationTokenSource;

        private const int DefaultBaudRate = 115200;

        public event Action<TelemetryPacket>? OnTelemetryReceived;
        public event Action<string>? OnLogMessage;
        public event Action<bool>? OnConnectionChanged;

        public Task<bool> ConnectAsync(string connectionString)
        {
            if (string.IsNullOrWhiteSpace(connectionString)) return Task.FromResult(false);

            lock (_lock)
            {
                if (_isRunning || (_serialPort != null && _serialPort.IsOpen)) return Task.FromResult(false);
            }

            try
            {
                _cancellationTokenSource = new CancellationTokenSource();
                _serialPort = new SerialPort(connectionString, DefaultBaudRate, Parity.None, 8, StopBits.One);
                _serialPort.ReadBufferSize = 1024;
                _serialPort.WriteBufferSize = 1024;
                _serialPort.DataReceived += SerialPort_DataReceived;
                _serialPort.Open();
                _isRunning = true;

                OnConnectionChanged?.Invoke(true);
                Log($"🔌 Подключение к ESP32 по COM-порту: {_serialPort.PortName}");
                return Task.FromResult(true);
            }
            catch (Exception ex)
            {
                Log($"❌ Ошибка подключения к COM-порту: {ex.Message}");
                Cleanup();
                return Task.FromResult(false);
            }
        }

        public void Disconnect()
        {
            lock (_lock) { if (!_isRunning) return; _isRunning = false; }
            _cancellationTokenSource?.Cancel();
            Cleanup();
            OnConnectionChanged?.Invoke(false);
        }

        private void Cleanup()
        {
            if (_serialPort != null)
            {
                if (_serialPort.IsOpen) { _serialPort.DataReceived -= SerialPort_DataReceived; _serialPort.Close(); }
                _serialPort.Dispose();
                _serialPort = null;
            }
        }

        private void SerialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            var port = (SerialPort)sender;
            int bytesToRead = port.BytesToRead;
            if (bytesToRead < TelemetryPacket.ExpectedSize) return;

            byte[] buffer = new byte[bytesToRead];
            int bytesRead = port.Read(buffer, 0, bytesToRead);

            for (int i = 0; i <= bytesRead - TelemetryPacket.ExpectedSize; i++)
            {
                if (buffer[i] == 0xAA && buffer[i + 1] == 0x55)
                {
                    byte[] packetBuffer = new byte[TelemetryPacket.ExpectedSize];
                    Array.Copy(buffer, i, packetBuffer, 0, TelemetryPacket.ExpectedSize);
                    if (ValidateAndProcessPacket(packetBuffer)) break;
                }
            }
        }

        private bool ValidateAndProcessPacket(byte[] data)
        {
            ushort receivedCrc = (ushort)(data[101] | (data[102] << 8));
            ushort calculatedCrc = Crc16Helper.Calculate(data, data.Length - 2);
            if (calculatedCrc != receivedCrc) return false;

            var packet = BytesToStruct<TelemetryPacket>(data);
            OnTelemetryReceived?.Invoke(packet);
            return true;
        }

        public void SendCommand(CommandId cmd, byte[]? payload = null)
        {
            lock (_lock) { if (!_isRunning || _serialPort == null || !_serialPort.IsOpen) return; }
            byte[] packet = BuildCommandPacket(cmd, payload);
            try { _serialPort!.Write(packet, 0, packet.Length); }
            catch (Exception ex) { Log($"❌ Ошибка отправки по COM: {ex.Message}"); }
        }

        private byte[] BuildCommandPacket(CommandId cmdId, byte[]? payload = null)
        {
            int payloadLen = payload?.Length ?? 0;
            int totalLen = 2 + 1 + payloadLen + 2;
            byte[] packet = new byte[totalLen];
            packet[0] = 0xAA; packet[1] = 0x55; packet[2] = (byte)cmdId;
            if (payload != null && payloadLen > 0) Array.Copy(payload, 0, packet, 3, payloadLen);
            ushort crc = Crc16Helper.Calculate(packet, totalLen - 2);
            packet[totalLen - 2] = (byte)(crc & 0xFF);
            packet[totalLen - 1] = (byte)((crc >> 8) & 0xFF);
            return packet;
        }

        private static T BytesToStruct<T>(byte[] bytes) where T : struct
        {
            int size = System.Runtime.InteropServices.Marshal.SizeOf<T>();
            IntPtr ptr = System.Runtime.InteropServices.Marshal.AllocHGlobal(size);
            try { System.Runtime.InteropServices.Marshal.Copy(bytes, 0, ptr, Math.Min(size, bytes.Length)); return System.Runtime.InteropServices.Marshal.PtrToStructure<T>(ptr); }
            finally { System.Runtime.InteropServices.Marshal.FreeHGlobal(ptr); }
        }

        private void Log(string msg) => OnLogMessage?.Invoke($"[SERIAL] {msg}");
    }
}