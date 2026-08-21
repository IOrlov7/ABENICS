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
            }
            _activeInterface = selectedInterface;
            _activeInterface.OnTelemetryReceived += OnTelemetryReceived;
            _activeInterface.OnLogMessage += OnLogMessage;
            _activeInterface.OnConnectionChanged += OnConnectionChanged;
        }

        private void OnTelemetryReceived(TelemetryPacket packet)
        {
            _sensorData.UpdateFromPacket(packet);
            
            // 🔴 ДИАГНОСТИКА: Печать каждые 50 пакетов (~1 раз в секунду при 50 Гц)
            if (packet.PacketId % 50 == 0)
            {
                double quatNorm = Math.Sqrt(packet.QuatW * packet.QuatW + packet.QuatX * packet.QuatX + packet.QuatY * packet.QuatY + packet.QuatZ * packet.QuatZ);
                Debug.WriteLine($"[MainWindow] 📦 Пакет #{packet.PacketId}: Quat=({packet.QuatW:F4}, {packet.QuatX:F4}, {packet.QuatY:F4}, {packet.QuatZ:F4}), Norm={quatNorm:F4}");
                
                if (Math.Abs(quatNorm - 1.0) > 0.1)
                {
                    Debug.WriteLine($"[MainWindow] ⚠️ ВНИМАНИЕ: Кватернион НЕ нормализован на уровне пакета!");
                }
                if (packet.QuatW == 1.0f && packet.QuatX == 0.0f && packet.QuatY == 0.0f && packet.QuatZ == 0.0f)
                {
                    Debug.WriteLine($"[MainWindow] ⚠️ ВНИМАНИЕ: Кватернион = (1,0,0,0) — фильтр Madgwick на ESP32 не обновляет данные!");
                }
            }

            UiDispatcher.Invoke(() =>
            {
                // 🔴 ДИАГНОСТИКА: Проверка перед вызовом
                if (Visualizer3D != null)
                {
                    Visualizer3D.UpdateRotation(packet.QuatW, packet.QuatX, packet.QuatY, packet.QuatZ);
                }
                else
                {
                    Debug.WriteLine("[MainWindow] ❌ Visualizer3D == null! Проверь x:Name=\"Visualizer3D\" в MainWindow.xaml");
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