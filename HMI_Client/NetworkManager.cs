// NetworkManager.cs
using System;
using System.IO.Ports;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace IMU_Visualizer
{
    public class NetworkManager : IDisposable
    {
        public event Action<SensorData>  DataReceived;
        public event Action<string>      LogMessage;
        public event Action<string>      StatusChanged;

        // ──────────────────── UDP ────────────────────
        private UdpClient        _udpClient;
        private CancellationTokenSource _udpCts;
        private Task             _udpTask;

        public int UdpPort { get; set; } = 8888;

        public void StartUdp()
        {
            StopUdp();
            try
            {
                var endpoint = new IPEndPoint(IPAddress.Any, UdpPort);
                _udpClient = new UdpClient();
                _udpClient.Client.SetSocketOption(
                    SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
                _udpClient.Client.Bind(endpoint);

                _udpCts = new CancellationTokenSource();
                _udpTask = Task.Run(() => UdpLoop(_udpCts.Token));

                Log($"UDP слушает на порту {UdpPort}");
                StatusChanged?.Invoke($"UDP :{UdpPort} ✓");
            }
            catch (Exception ex)
            {
                Log($"UDP ошибка: {ex.Message}");
                StatusChanged?.Invoke("UDP ✗");
            }
        }

        public void StopUdp()
        {
            _udpCts?.Cancel();
            try { _udpClient?.Close(); } catch { }
            _udpClient = null;
            _udpCts = null;
        }

        private async Task UdpLoop(CancellationToken ct)
        {
            while (!ct.IsCancellationRequested)
            {
                try
                {
                    UdpReceiveResult result = await _udpClient.ReceiveAsync();
                    string line = Encoding.UTF8.GetString(result.Buffer).Trim();
                    Log($"UDP ← {result.RemoteEndPoint}: {line.Substring(0, Math.Min(60, line.Length))}");
                    if (TryParse(line, out var data))
                        DataReceived?.Invoke(data);
                }
                catch (ObjectDisposedException) { break; }
                catch (SocketException) when (ct.IsCancellationRequested) { break; }
                catch (Exception ex) { Log($"UDP ошибка: {ex.Message}"); }
            }
        }

        // ──────────────────── COM-порт (ИСПРАВЛЕНО) ────────────────────
        private SerialPort _serial;
        private StringBuilder _serialBuffer = new StringBuilder();

        public string[] GetAvailablePorts() => SerialPort.GetPortNames();

        public void StartSerial(string portName, int baudRate = 115200)
        {
            StopSerial();
            try
            {
                _serial = new SerialPort(portName, baudRate)
                {
                    ReadTimeout = 2000,  // ★ Увеличен до 2 сек
                    WriteTimeout = 1000,
                    DtrEnable = true,
                    RtsEnable = true,
                    ReadBufferSize = 4096
                };
                _serial.DataReceived += Serial_DataReceived;
                _serial.Open();

                Log($"COM {portName} открыт @ {baudRate}");
                StatusChanged?.Invoke($"{portName} ✓");
            }
            catch (Exception ex)
            {
                Log($"COM ошибка: {ex.Message}");
                StatusChanged?.Invoke("COM ✗");
            }
        }

        public void StopSerial()
        {
            if (_serial == null) return;
            try
            {
                _serial.DataReceived -= Serial_DataReceived;
                if (_serial.IsOpen) _serial.Close();
            }
            catch { }
            _serial = null;
            _serialBuffer.Clear();
        }

        private void Serial_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                // ★ Читаем ВСЁ, что есть в буфере
                string incoming = _serial.ReadExisting();
                
                // Отладка: выводим сырые данные (первые 100 символов)
                if (incoming.Length > 0)
                {
                    string preview = incoming.Length > 100 
                        ? incoming.Substring(0, 100) + "..." 
                        : incoming;
                    Log($"COM RAW: {preview.Replace("\r", "\\r").Replace("\n", "\\n")}");
                }

                _serialBuffer.Append(incoming);

                // ★ Разбираем по строкам (поддерживаем \r\n и \n)
                string buffer = _serialBuffer.ToString();
                int idx;
                while ((idx = buffer.IndexOfAny(new[] { '\r', '\n' })) >= 0)
                {
                    string line = buffer.Substring(0, idx).Trim();
                    buffer = buffer.Substring(idx + 1).TrimStart('\r', '\n');

                    if (!string.IsNullOrEmpty(line))
                    {
                        if (TryParse(line, out var data))
                            DataReceived?.Invoke(data);
                        else
                            Log($"COM не распознано: {line}");
                    }
                }
                _serialBuffer.Clear();
                _serialBuffer.Append(buffer);
            }
            catch (Exception ex)
            {
                Log($"COM ошибка чтения: {ex.Message}");
            }
        }

        /// <summary>
        /// Отправка команды на ESP32 (через COM).
        /// Команды: MODE SERIAL|AP|STA, IP x.x.x.x, SAVE, RESET
        /// </summary>
        public void SendCommand(string cmd)
        {
            if (_serial == null || !_serial.IsOpen)
            {
                Log("COM не открыт — команда не отправлена");
                return;
            }
            _serial.WriteLine(cmd);
            Log($"COM → {cmd}");
        }

        // ──────────────────── Парсер ────────────────────
        private static bool TryParse(string line, out SensorData d)
        {
            d = default;
            if (string.IsNullOrEmpty(line) || !line.StartsWith("DATA|"))
                return false;

            var parts = line.Split('|');
            if (parts.Length < 12) return false;

            try
            {
                for (int i = 1; i < parts.Length; i++)
                {
                    var kv = parts[i].Split(':');
                    if (kv.Length != 2) continue;
                    float v = float.Parse(kv[1], System.Globalization.CultureInfo.InvariantCulture);

                    switch (kv[0])
                    {
                        case "T":  d.Temperature = v; break;
                        case "AX": d.AccX = v; break;
                        case "AY": d.AccY = v; break;
                        case "AZ": d.AccZ = v; break;
                        case "QW": d.QuatW = v; break;
                        case "QX": d.QuatX = v; break;
                        case "QY": d.QuatY = v; break;
                        case "QZ": d.QuatZ = v; break;
                        case "R":  d.Roll  = v; break;
                        case "P":  d.Pitch = v; break;
                        case "Y":  d.Yaw   = v; break;
                    }
                }
                return true;
            }
            catch { return false; }
        }

        private void Log(string msg)
        {
            string stamped = $"[{DateTime.Now:HH:mm:ss.fff}] {msg}";
            LogMessage?.Invoke(stamped);
        }

        public void Dispose()
        {
            StopUdp();
            StopSerial();
        }
    }
}