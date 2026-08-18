using System;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;
// НЕ НУЖНО никакого дополнительного using — оба файла в namespace HMI_Client

namespace HMI_Client
{
    public class NetworkManager
    {
        private UdpClient? _receiveClient;
        private UdpClient? _sendClient;
        private IPEndPoint? _esp32EndPoint;
        private bool _isRunning;

        public event Action<TelemetryPacket>? OnTelemetryReceived;
        public event Action<string>? OnLogMessage;

        private const int ReceivePort = 8888;
        private const int SendPort = 8889;

        public string GetEsp32Ip() => _esp32EndPoint?.Address.ToString() ?? "Не определён";

        public void Start()
        {
            _isRunning = true;
            _receiveClient = new UdpClient(ReceivePort);
            _sendClient = new UdpClient();

            Log("Сетевой менеджер запущен. Ожидание телеметрии на порту " + ReceivePort);
            _ = ReceiveLoopAsync();
        }

        public void Stop()
        {
            _isRunning = false;
            _receiveClient?.Close();
            _sendClient?.Close();
        }

        public void ConnectToEspManual(string ip)
        {
            _esp32EndPoint = new IPEndPoint(IPAddress.Parse(ip), SendPort);
            Log($"🔌 Ручное подключение к ESP32: {ip}:{SendPort}");
            SendCommand(CommandId.CMD_IDLE); // Отправляем пустую команду, чтобы ESP32 запомнил наш IP
        }

        private async Task ReceiveLoopAsync()
        {
            while (_isRunning)
            {
                try
                {
                    UdpReceiveResult result = await _receiveClient!.ReceiveAsync();
                    byte[] data = result.Buffer;

                    if (PacketValidator.Validate(data))
                    {
                        if (_esp32EndPoint == null)
                        {
                            _esp32EndPoint = result.RemoteEndPoint;
                            Log($"ESP32 обнаружен: {_esp32EndPoint.Address}");
                        }

                        TelemetryPacket packet = BytesToStruct<TelemetryPacket>(data);
                        OnTelemetryReceived?.Invoke(packet);
                    }
                }
                catch (ObjectDisposedException)
                {
                    break;
                }
                catch (Exception ex)
                {
                    Log($"Ошибка приёма UDP: {ex.Message}");
                }
            }
        }

        public void SendCommand(CommandId cmdId, byte[]? payload = null)
        {
            if (_esp32EndPoint == null)
            {
                Log("Ошибка: IP-адрес ESP32 ещё не определён.");
                return;
            }

            byte[] packet = PacketValidator.BuildCommandPacket(cmdId, payload);
            _sendClient?.SendAsync(packet, packet.Length, _esp32EndPoint);
            Log($"Отправлена команда: {cmdId}");
        }

        public void SendEStop() => SendCommand(CommandId.CMD_ESTOP);
        public void SendHome() => SendCommand(CommandId.CMD_HOME);
        public void SendCalibrate() => SendCommand(CommandId.CMD_CALIBRATE);
        public void SendResume() => SendCommand(CommandId.CMD_RESUME);

        private void Log(string msg)
        {
            OnLogMessage?.Invoke($"[{DateTime.Now:HH:mm:ss.fff}] {msg}");
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
            finally
            {
                System.Runtime.InteropServices.Marshal.FreeHGlobal(ptr);
            }
        }
    }
}