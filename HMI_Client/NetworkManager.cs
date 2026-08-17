using System;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

public class NetworkManager
{
    private UdpClient _udpClient;
    private const int TELEMETRY_PORT = 8888;
    public event Action<TelemetryPacket> OnTelemetryReceived;

    public async Task StartListeningAsync()
    {
        _udpClient = new UdpClient(TELEMETRY_PORT);
        Console.WriteLine($"[UDP] Listening on port {TELEMETRY_PORT}...");

        while (true)
        {
            try
            {
                UdpReceiveResult result = await _udpClient.ReceiveAsync();
                
                if (result.Buffer.Length == Marshal.SizeOf<TelemetryPacket>())
                {
                    TelemetryPacket packet = FromBytes<TelemetryPacket>(result.Buffer);
                    
                    // Проверка заголовка
                    if (packet.Header[0] == 'T' && packet.Header[1] == 'E') 
                    {
                        OnTelemetryReceived?.Invoke(packet); // Отправляем в MainWindow
                    }
                }
            }
            catch (Exception ex) { Console.WriteLine($"[UDP Error] {ex.Message}"); }
        }
    }

    // Магия C#: превращаем массив байтов в структуру без парсинга строк!
    private static T FromBytes<T>(byte[] data) where T : struct
    {
        GCHandle handle = GCHandle.Alloc(data, GCHandleType.Pinned);
        T theStructure = (T)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(T));
        handle.Free();
        return theStructure;
    }
}