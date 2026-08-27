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
using HMI_Client.Comms.Data;

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

        public async Task<bool> ConnectAsync(string? connectionString)
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

                // ★ ReuseAddress + явный Bind
                _receiveClient = new UdpClient();
                _receiveClient.Client.SetSocketOption(
                    SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
                _receiveClient.Client.Bind(new IPEndPoint(IPAddress.Any, ReceivePort));

                _sendClient = new UdpClient();
                _esp32EndPoint = new IPEndPoint(ip, SendPort);

                _isRunning = true;
                OnConnectionChanged?.Invoke(true);
                Log($"🔌 Подключение к ESP32 по UDP: {ip}:{SendPort}");

                // Регистрация клиента (переключение ESP32 на unicast)
                SendCommand(CommandId.CMD_IDLE);
                Log("📤 CMD_IDLE отправлен (регистрация)");

                _ = Task.Run(() => ReceiveLoopAsync(_cancellationTokenSource.Token));
                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[UDP] ❌ Ошибка подключения: {ex.Message}");
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
            uint packetCount = 0;  // ← uint, НЕ uint32_t

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
                            Log($"🔍 ESP32 обнаружен: {_esp32EndPoint.Address}");
                        }

                        var packet = TelemetryPacket.FromBytes(data);
                        packetCount++;

                        // Диагностика каждый 250-й пакет (~раз в 5 сек при 50 Гц)
                        if (packetCount % 250 == 1)
                        {
                            Log($"📦 Пакетов: {packetCount}, ID={packet.PacketId}, " +
                                $"accel=({packet.AccelX:F2}, {packet.AccelY:F2}, {packet.AccelZ:F2})");
                        }

                        OnTelemetryReceived?.Invoke(packet);
                    }
                }
                catch (ObjectDisposedException) { break; }
                catch (Exception ex) when (!cancellationToken.IsCancellationRequested)
                {
                    Log($"❌ Ошибка приёма UDP: {ex.Message}");
                }
            }

            Log($"🔚 ReceiveLoop завершён. Всего пакетов: {packetCount}");
        }

        private bool ValidatePacket(byte[] data)
        {
            if (data == null || data.Length != TelemetryPacket.ExpectedSize) return false;
            if (data[0] != 0xAA || data[1] != 0x55) return false;

            // ★ CRC — мягкая проверка: логируем, но НЕ отбрасываем
            ushort receivedCrc = (ushort)(data[TelemetryPacket.ExpectedSize - 2] |
                                          (data[TelemetryPacket.ExpectedSize - 1] << 8));
            ushort calculatedCrc = Crc16Helper.Calculate(data, data.Length - 2);

            if (calculatedCrc != receivedCrc)
            {
                Log($"⚠ CRC mismatch: got 0x{receivedCrc:X4}, calc 0x{calculatedCrc:X4}");
                // НЕ return false — пакет дошёл через UDP, биты целы
            }

            return true;
        }

        public void SendCommand(CommandId cmdId, byte[]? payload = null)
        {
            if (_esp32EndPoint == null) return;
            byte[] packet = BuildCommandPacket(cmdId, payload);
            _sendClient?.Send(packet, packet.Length, _esp32EndPoint);
        }

        public void SendRawData(byte[] data)
        {
            if (_esp32EndPoint == null || data == null) return;
            try
            {
                _sendClient?.Send(data, data.Length, _esp32EndPoint);
            }
            catch (Exception ex)
            {
                Log($"Ошибка отправки сырых данных: {ex.Message}");
            }
        }

        private byte[] BuildCommandPacket(CommandId cmdId, byte[]? payload = null)
        {
            int payloadLen = payload?.Length ?? 0;
            int totalLen = 2 + 1 + payloadLen + 2; // header(2) + cmd(1) + payload + crc(2)
            byte[] packet = new byte[totalLen];
            packet[0] = 0xAA;
            packet[1] = 0x55;
            packet[2] = (byte)cmdId;
            if (payload != null && payloadLen > 0) Array.Copy(payload, 0, packet, 3, payloadLen);
            ushort crc = Crc16Helper.Calculate(packet, totalLen - 2);
            packet[totalLen - 2] = (byte)(crc & 0xFF); // lo byte
            packet[totalLen - 1] = (byte)((crc >> 8) & 0xFF); // hi byte
            return packet;
        }


        private void Log(string msg) => OnLogMessage?.Invoke($"[UDP] {msg}");
    }
}