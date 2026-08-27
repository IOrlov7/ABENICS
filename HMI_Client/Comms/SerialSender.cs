using System;
using System.IO.Ports;
using System.Threading;
using System.Threading.Tasks;
using HMI_Client.Comms.Data;
using HMI_Client.Utils;
using HMI_Client.Comms.Data;

namespace HMI_Client.Comms
{
    public class SerialSender : ICommInterface, IDisposable
    {
        private SerialPort? _serialPort;
        private CancellationTokenSource? _cts;
        private Task? _readTask;
        private byte[] _buffer = new byte[2048];
        private int _bufferPos = 0;
        private long _totalBytesRead = 0;   // ★ ДИАГНОСТИКА: сколько байт прочитано
        private uint _diagCount = 0;         // ★ ДИАГНОСТИКА: счётчик вызовов ProcessBuffer
        private DateTime _lastCrcLog = DateTime.MinValue;
        private long _totalCrcFails = 0;     // ★ ДИАГНОСТИКА: всего CRC-мисматчей

        public string PortName { get; private set; } = "COM7";
        public int BaudRate { get; set; } = 115200;

        // События интерфейса ICommInterface
        public event Action<TelemetryPacket>? OnTelemetryReceived;
        public event Action<string>? OnLogMessage;
        public event Action<bool>? OnConnectionChanged;

        public int PacketsReceived { get; private set; }
        public int PacketsCorrupted { get; private set; }
        public ushort LastPacketId { get; private set; }
        public DateTime LastPacketTime { get; private set; }
        public bool IsConnected => _serialPort?.IsOpen ?? false;

        // ============================================================
        //  ICommInterface: ConnectAsync
        // ============================================================
        public Task<bool> ConnectAsync(string? connectionString)
        {
            string portName = connectionString ?? PortName;
            return Task.FromResult(Connect(portName, BaudRate));
        }

        public bool Connect(string portName, int baudRate)
        {
            try
            {
                PortName = portName;
                BaudRate = baudRate;

                _serialPort = new SerialPort(portName, baudRate, Parity.None, 8, StopBits.One)
                {
                    ReadTimeout = 100,
                    WriteTimeout = 100,
                    ReadBufferSize = 4096,
                    WriteBufferSize = 4096
                };

                _serialPort.Open();
                _cts = new CancellationTokenSource();
                _readTask = Task.Run(() => ReadLoop(_cts.Token));

                OnLogMessage?.Invoke($"[Serial] Подключено: {portName} @ {baudRate}");
                OnConnectionChanged?.Invoke(true);
                return true;
            }
            catch (Exception ex)
            {
                OnLogMessage?.Invoke($"[Serial] Ошибка подключения к {portName}: {ex.Message}");
                OnConnectionChanged?.Invoke(false);
                return false;
            }
        }

        public void Disconnect()
        {
            _cts?.Cancel();
            _readTask?.Wait(1000);

            if (_serialPort?.IsOpen == true)
            {
                _serialPort.Close();
                _serialPort.Dispose();
                _serialPort = null;
            }

            OnLogMessage?.Invoke("[Serial] Отключено");
            OnConnectionChanged?.Invoke(false);
        }

        public void Dispose() => Disconnect();

        // ============================================================
        //  Цикл чтения
        // ============================================================
        private void ReadLoop(CancellationToken token)
        {
            OnLogMessage?.Invoke($"[Serial] ReadLoop запущен, port open={_serialPort?.IsOpen}");
            _diagCount = 0;
            while (!token.IsCancellationRequested && _serialPort?.IsOpen == true)
            {
                try
                {
                    int bytesRead = _serialPort.Read(_buffer, _bufferPos, _buffer.Length - _bufferPos);
                    if (bytesRead <= 0) continue;

                    _totalBytesRead += bytesRead;
                    _bufferPos += bytesRead;
                    ProcessBuffer();
                }
                catch (TimeoutException) { }
                catch (Exception ex)
                {
                    if (!token.IsCancellationRequested)
                        OnLogMessage?.Invoke($"[Serial] Ошибка чтения: {ex.Message}");
                    break;
                }
            }
        }

        private void ProcessBuffer()
        {
            _diagCount++;
            while (_bufferPos >= 2)
            {
                int headerIndex = FindHeader();
                if (headerIndex < 0)
                {
                    if (_bufferPos > 0)
                    {
                        _buffer[0] = _buffer[_bufferPos - 1];
                        _bufferPos = 1;
                    }
                    return;
                }

                if (headerIndex > 0)
                {
                    Array.Copy(_buffer, headerIndex, _buffer, 0, _bufferPos - headerIndex);
                    _bufferPos -= headerIndex;
                }

                if (_bufferPos < TelemetryPacket.ExpectedSize)
                    return;

                // CRC little-endian: [lo] [hi]
                // CRC расположен в двух последних байтах пакета
                // (offset ExpectedSize-2 / ExpectedSize-1, для 119-байтового пакета — 117/118)
                ushort receivedCRC = (ushort)(_buffer[TelemetryPacket.ExpectedSize - 2] |
                                              (_buffer[TelemetryPacket.ExpectedSize - 1] << 8));
                ushort calcCRC = Crc16Helper.Calculate(_buffer, TelemetryPacket.ExpectedSize - 2);

                if (receivedCRC == calcCRC)
                {
                    try
                    {
                        var packet = TelemetryPacket.FromBytes(_buffer);
                        LastPacketId = (ushort)packet.PacketId;
                        LastPacketTime = DateTime.Now;
                        PacketsReceived++;
                        OnTelemetryReceived?.Invoke(packet);
                    }
                    catch (Exception ex)
                    {
                        OnLogMessage?.Invoke($"[Serial] Ошибка десериализации: {ex.Message}");
                        PacketsCorrupted++;
                    }
                }
                else
                {
                    PacketsCorrupted++;
                    _totalCrcFails++;
                    // ★ ДИАГНОСТИКА: логируем CRC-мисматч не чаще 1 раза в секунду
                    var now = DateTime.Now;
                    if ((now - _lastCrcLog).TotalSeconds >= 1.0)
                    {
                        _lastCrcLog = now;
                        OnLogMessage?.Invoke(
                            $"[Serial] ⚠ CRC mismatch от ESP32: got 0x{receivedCRC:X4}, calc 0x{calcCRC:X4}, " +
                            $"bufPos={_bufferPos}, hdr=0x{_buffer[0]:X2} 0x{_buffer[1]:X2}, totBytes={_totalBytesRead}");
                    }
                }

                int remaining = _bufferPos - TelemetryPacket.ExpectedSize;
                if (remaining > 0)
                {
                    Array.Copy(_buffer, TelemetryPacket.ExpectedSize, _buffer, 0, remaining);
                }
                _bufferPos = remaining;

                // ★ ДИАГНОСТИКА: раз в 200 пакетов показываем статистику приёма
                if (_diagCount % 200 == 0)
                {
                    OnLogMessage?.Invoke($"[Serial] DIAG: read={_totalBytesRead}B, parsed={PacketsReceived}, crcFail={_totalCrcFails}, bufPos={_bufferPos}");
                }
            }
        }

        private int FindHeader()
        {
            for (int i = 0; i <= _bufferPos - 2; i++)
            {
                if (_buffer[i] == 0xAA && _buffer[i + 1] == 0x55)
                    return i;
            }
            return -1;
        }

        // ============================================================
        //  ICommInterface: SendCommand
        // ============================================================
        public void SendCommand(CommandId cmd, byte[]? payload = null)
        {
            if (_serialPort?.IsOpen != true)
            {
                OnLogMessage?.Invoke("[Serial] Попытка отправки команды без подключения");
                return;
            }

            int payloadLen = payload?.Length ?? 0;
            int packetLen = 3 + payloadLen + 2;
            byte[] cmdPacket = new byte[packetLen];

            cmdPacket[0] = 0xAA;
            cmdPacket[1] = 0x55;
            cmdPacket[2] = (byte)cmd;

            if (payload != null)
                Array.Copy(payload, 0, cmdPacket, 3, payloadLen);

            ushort crc = Crc16Helper.Calculate(cmdPacket, packetLen - 2);
            cmdPacket[packetLen - 2] = (byte)(crc & 0xFF);
            cmdPacket[packetLen - 1] = (byte)((crc >> 8) & 0xFF);

            try
            {
                _serialPort.Write(cmdPacket, 0, cmdPacket.Length);
            }
            catch (Exception ex)
            {
                OnLogMessage?.Invoke($"[Serial] Ошибка отправки: {ex.Message}");
            }
        }

        public void SendRawData(byte[] data)
        {
            if (_serialPort?.IsOpen != true || data == null)
            {
                OnLogMessage?.Invoke("[Serial] Попытка отправки без подключения");
                return;
            }

            try
            {
                _serialPort.Write(data, 0, data.Length);
            }
            catch (Exception ex)
            {
                OnLogMessage?.Invoke($"[Serial] Ошибка отправки: {ex.Message}");
            }
        }

        public static string[] GetAvailablePorts()
        {
            try { return SerialPort.GetPortNames(); }
            catch { return Array.Empty<string>(); }
        }

        public static int[] GetStandardBaudRates()
        {
            return new[] { 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600 };
        }
    }
}