// File: HMI_Client/Comms/UdpSender.cs
//
// НАЗНАЧЕНИЕ:
// Реализация интерфейса ICommInterface для получения данных по протоколу UDP.
// Отвечает за открытие UDP-сокетов, прием пакетов, проверку CRC и отправку команд.
//
// ОТВЕЧАЕТ ЗА:
// - Подключение к ESP32 по Wi-Fi (UDP).
// - Прием и валидация UDP-пакетов.
// - Отправка команд управления на ESP32.

using System;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;
using System.Threading;
using HMI_Client.Comms.Data;
using HMI_Client.Utils;

namespace HMI_Client.Comms
{
    public class UdpSender : ICommInterface
    {
        private UdpClient? _receiveClient;
        private UdpClient? _sendClient;
        private IPEndPoint? _esp32EndPoint;
        private bool _isRunning;
        private readonly object _lock = new object();
        private CancellationTokenSource? _cancellationTokenSource;

        private const int ReceivePort = 8888;
        private const int SendPort = 8889;

        public event Action<TelemetryPacket>? OnTelemetryReceived;
        public event Action<string>? OnLogMessage;
        public event Action<bool>? OnConnectionChanged;

        public async Task<bool> ConnectAsync(string connectionString)
        {
            lock (_lock)
            {
                if (_isRunning) return false;
            }

            if (!IPAddress.TryParse(connectionString, out var ip))
            {
                Log("❌ Неверный формат IP-адреса.");
                return false;
            }

            try
            {
                _cancellationTokenSource = new CancellationTokenSource();
                _receiveClient = new UdpClient(ReceivePort);
                _sendClient = new UdpClient();
                _esp32EndPoint = new IPEndPoint(ip, SendPort);
                _isRunning = true;

                OnConnectionChanged?.Invoke(true);
                Log($"🔌 Подключение к ESP32 по UDP: {ip}:{SendPort}");
                SendCommand(CommandId.CMD_IDLE);

                _ = Task.Run(() => ReceiveLoopAsync(_cancellationTokenSource.Token));
                return true;
            }
            catch (Exception ex)
            {
                Log($"❌ Ошибка подключения по UDP: {ex.Message}");
                Cleanup();
                return false;
            }
        }

        public void Disconnect()
        {
            lock (_lock)
            {
                if (!_isRunning) return;
                _isRunning = false;
            }
            _cancellationTokenSource?.Cancel();
            Cleanup();
            OnConnectionChanged?.Invoke(false);
        }

        private void Cleanup()
        {
            _receiveClient?.Close();
            _sendClient?.Close();
            _receiveClient = null;
            _sendClient = null;
            _esp32EndPoint = null;
        }

        private async Task ReceiveLoopAsync(CancellationToken cancellationToken)
        {
            while (!cancellationToken.IsCancellationRequested && _isRunning)
            {
                try
                {
                    var result = await _receiveClient!.ReceiveAsync();
                    byte[] data = result.Buffer;

                    if (ValidatePacket(data))
                    {
                        if (_esp32EndPoint == null)
                        {
                            _esp32EndPoint = new IPEndPoint(result.RemoteEndPoint.Address, SendPort);
                            Log($"ESP32 обнаружен: {_esp32EndPoint.Address}");
                        }

                        var packet = BytesToStruct<TelemetryPacket>(data);
                        OnTelemetryReceived?.Invoke(packet);
                    }
                }
                catch (ObjectDisposedException) { break; }
                catch (Exception ex) when (!cancellationToken.IsCancellationRequested)
                {
                    Log($"❌ Ошибка приёма UDP: {ex.Message}");
                }
            }
        }

        private bool ValidatePacket(byte[] data)
        {
            if (data == null || data.Length != TelemetryPacket.ExpectedSize) return false;
            if (data[0] != 0xAA || data[1] != 0x55) return false;

            ushort receivedCrc = (ushort)(data[101] | (data[102] << 8));
            ushort calculatedCrc = Crc16Helper.Calculate(data, data.Length - 2);

            return calculatedCrc == receivedCrc;
        }

        public void SendCommand(CommandId cmdId, byte[]? payload = null)
        {
            if (_esp32EndPoint == null) return;
            byte[] packet = BuildCommandPacket(cmdId, payload);
            _sendClient?.Send(packet, packet.Length, _esp32EndPoint);
        }

        private byte[] BuildCommandPacket(CommandId cmdId, byte[]? payload = null)
        {
            int payloadLen = payload?.Length ?? 0;
            int totalLen = 2 + 1 + payloadLen + 2;
            byte[] packet = new byte[totalLen];
            packet[0] = 0xAA;
            packet[1] = 0x55;
            packet[2] = (byte)cmdId;
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
            try
            {
                System.Runtime.InteropServices.Marshal.Copy(bytes, 0, ptr, Math.Min(size, bytes.Length));
                return System.Runtime.InteropServices.Marshal.PtrToStructure<T>(ptr);
            }
            finally { System.Runtime.InteropServices.Marshal.FreeHGlobal(ptr); }
        }

        private void Log(string msg) => OnLogMessage?.Invoke($"[UDP] {msg}");
    }
}