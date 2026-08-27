// File: HMI_Client/Visual/MainWindow.xaml.cs
using System;
using System.Windows;
using System.Windows.Threading;
using System.Diagnostics;
using HMI_Client.Comms;
using HMI_Client.Comms.Data;
using HMI_Client.Data;
using HMI_Client.Utils;

namespace HMI_Client.Visual
{
    public partial class MainWindow : Window
    {
        private CommandDispatcher _commandDispatcher;
        private SensorData _sensorData;
        private TelemetryLogger _telemetryLogger;
        private ICommInterface? _activeInterface;
        private int _packetCount = 0;
        private DateTime _lastRateCheck = DateTime.Now;

        public MainWindow()
        {
            InitializeComponent();
            _commandDispatcher = new CommandDispatcher();
            _sensorData = new SensorData();
            _telemetryLogger = new TelemetryLogger();
            ControlPanel.SetCommandDispatcher(_commandDispatcher);
            ControlPanel.OnInterfaceSelected += OnInterfaceSelected;
            var timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(200) };
            timer.Tick += (s, e) => UpdateStatus();
            timer.Start();
            LogView.AppendLog("ABENICS HMI Client запущен.");
        }

        private void OnInterfaceSelected(ICommInterface selectedInterface)
        {
            if (_activeInterface != null)
            {
                _activeInterface.OnTelemetryReceived -= OnTelemetryReceived;
                _activeInterface.OnLogMessage -= OnLogMessage;
                _activeInterface.OnConnectionChanged -= OnConnectionChanged;
                _activeInterface.Disconnect();
            }
            _activeInterface = selectedInterface;
            _activeInterface.OnTelemetryReceived += OnTelemetryReceived;
            _activeInterface.OnLogMessage += OnLogMessage;
            _activeInterface.OnConnectionChanged += OnConnectionChanged;
            
            // 🔴 Логируем в LogView
            UiDispatcher.Invoke(() => 
                LogView.AppendLog($"[MAIN] Подключён интерфейс: {selectedInterface.GetType().Name}"));
        }

        private void OnTelemetryReceived(TelemetryPacket packet)
        {
            _sensorData.UpdateFromPacket(packet);
            
            // 🔴 ДИАГНОСТИКА: Первые 5 пакетов — выводим ВСЁ в LogView
            if (_packetCount < 5)
            {
                double quatNorm = Math.Sqrt(
                    packet.QuatW * packet.QuatW + 
                    packet.QuatX * packet.QuatX + 
                    packet.QuatY * packet.QuatY + 
                    packet.QuatZ * packet.QuatZ);
                
                UiDispatcher.Invoke(() =>
                {
                    LogView.AppendLog($"[QUAT] Пакет #{_packetCount}, ID={packet.PacketId}");
                    LogView.AppendLog($"[QUAT] W={packet.QuatW:F4}, X={packet.QuatX:F4}, Y={packet.QuatY:F4}, Z={packet.QuatZ:F4}");
                    LogView.AppendLog($"[QUAT] Norm={quatNorm:F4}");
                    LogView.AppendLog($"[QUAT] Euler=({packet.EulerRoll:F1}°, {packet.EulerPitch:F1}°, {packet.EulerYaw:F1}°)");
                    
                    if (packet.QuatW == 1.0f && packet.QuatX == 0.0f && 
                        packet.QuatY == 0.0f && packet.QuatZ == 0.0f)
                    {
                        LogView.AppendLog("[QUAT] ⚠️ ВНИМАНИЕ: Quat=(1,0,0,0) — Madgwick не обновляется!");
                    }
                });
            }
            
            // 🔴 Каждые 100 пакетов — краткий лог
            if (_packetCount % 100 == 0 && _packetCount > 0)
            {
                UiDispatcher.Invoke(() =>
                    LogView.AppendLog($"[QUAT] Пакет #{_packetCount}: W={packet.QuatW:F2}, X={packet.QuatX:F2}, Y={packet.QuatY:F2}, Z={packet.QuatZ:F2}"));
            }

            UiDispatcher.Invoke(() =>
            {
                // 🔴 Проверка Visualizer3D
                if (Visualizer3D == null && _packetCount == 0)
                {
                    LogView.AppendLog("[3D] ❌ Visualizer3D == null! Проверь x:Name в MainWindow.xaml");
                }
                else
                {
                    Visualizer3D.UpdateRotation(packet.QuatW, packet.QuatX, packet.QuatY, packet.QuatZ);
                }

                TxtRoll.Text = $"{packet.EulerRoll:F1}°";
                TxtPitch.Text = $"{packet.EulerPitch:F1}°";
                TxtYaw.Text = $"{packet.EulerYaw:F1}°";
                TxtRssi.Text = $"{packet.WifiRssi} dBm";
                TxtLastPacket.Text = $"ID: {packet.PacketId}, T: {packet.TimestampMs} ms";
                GraphsView.UpdateCharts(packet.AccelX, packet.AccelY, packet.AccelZ, packet.GyroX, packet.GyroY, packet.GyroZ);
            });

            _telemetryLogger.LogPacket(packet);
            _packetCount++;
        }

        private void OnLogMessage(string msg) => UiDispatcher.Invoke(() => LogView.AppendLog(msg));

        private void OnConnectionChanged(bool isConnected)
        {
            UiDispatcher.Invoke(() =>
            {
                TxtStatus.Text = isConnected ? "🟢 Подключено" : "🔴 Отключено";
                LogView.AppendLog(isConnected ? "[CONN] 🟢 Соединение установлено" : "[CONN] 🔴 Соединение разорвано");
                if (isConnected) _telemetryLogger.StartLogging();
                else _telemetryLogger.StopLogging();
            });
        }

        private void UpdateStatus()
        {
            var now = DateTime.Now;
            if ((now - _lastRateCheck).TotalSeconds >= 1.0)
            {
                TxtPacketRate.Text = $"{_packetCount} Hz";
                _packetCount = 0;
                _lastRateCheck = now;
            }
        }

        protected override void OnClosed(EventArgs e)
        {
            if (_activeInterface != null)
            {
                _activeInterface.OnTelemetryReceived -= OnTelemetryReceived;
                _activeInterface.OnLogMessage -= OnLogMessage;
                _activeInterface.OnConnectionChanged -= OnConnectionChanged;
                _activeInterface.Disconnect();
            }
            _telemetryLogger.StopLogging();
            base.OnClosed(e);
        }
    }
}