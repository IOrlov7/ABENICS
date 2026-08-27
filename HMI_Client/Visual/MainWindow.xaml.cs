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
        private bool _visualizerWarned = false;

        // ★ ОГРАНИЧЕНИЕ частоты отрисовки UI (иначе при 50 пак./с UI копит очередь и подлагивает).
        // Значение → ~30 Гц отрисовки, этого достаточно для плавной анимации.
        private readonly System.Diagnostics.Stopwatch _uiClock = System.Diagnostics.Stopwatch.StartNew();
        private long _lastRenderMs = 0;
        private const long UiRenderIntervalMs = 33; // ~30 кадров/с

        public MainWindow()
        {
            InitializeComponent();
            _commandDispatcher = new CommandDispatcher();
            _sensorData = new SensorData();
            _telemetryLogger = new TelemetryLogger();
            ControlPanel.SetCommandDispatcher(_commandDispatcher);
            PidTuner.SetCommandDispatcher(_commandDispatcher);
            PidTuner.OnLog += msg => UiDispatcher.Invoke(() => LogView.AppendLog(msg));
            ControlPanel.OnInterfaceSelected += OnInterfaceSelected;
            ControlPanel.OnInterfaceDisconnected += OnInterfaceDisconnected; // ★ НОВОЕ
            ControlPanel.OnLogInfo += msg => UiDispatcher.Invoke(() => LogView.AppendLog(msg)); // ★ НОВОЕ
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

        // ★ НОВОЕ: пользователь нажал «Отключиться» — прекращаем слушать канал,
        // отписываемся от всех событий интерфейса.
        private void OnInterfaceDisconnected()
        {
            if (_activeInterface != null)
            {
                _activeInterface.OnTelemetryReceived -= OnTelemetryReceived;
                _activeInterface.OnLogMessage -= OnLogMessage;
                _activeInterface.OnConnectionChanged -= OnConnectionChanged;
                _activeInterface = null;
            }
        }

        private void OnTelemetryReceived(TelemetryPacket packet)
        {
            _sensorData.UpdateFromPacket(packet);
            _packetCount++;

            // 🔴 ДИАГНОСТИКА: Первые 5 пакетов — выводим ВСЁ в LogView
            if (_packetCount <= 5)
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

            // ★ ФИЛЬТР: обновляем тяжёлый UI (3D, графики, PID) и логируем на диск
            // не чаще UiRenderIntervalMs (≈30 Гц). Остальные пакеты просто пропускаем —
            // приём чтения не блокируется, UI не копит очередь.
            var nowMs = _uiClock.ElapsedMilliseconds;
            if (_packetCount <= 5 || (nowMs - _lastRenderMs) >= UiRenderIntervalMs)
            {
                _lastRenderMs = nowMs;

                // 🔴 Каждые 100 пакетов — краткий лог
                if (_packetCount % 100 == 0 && _packetCount > 0)
                {
                    UiDispatcher.Invoke(() =>
                        LogView.AppendLog($"[QUAT] Пакет #{_packetCount}: W={packet.QuatW:F2}, X={packet.QuatX:F2}, Y={packet.QuatY:F2}, Z={packet.QuatZ:F2}"));
                }

                UiDispatcher.Invoke(() =>
                {
                    // 🔴 Проверка Visualizer3D
                    if (Visualizer3D == null)
                    {
                        if (!_visualizerWarned)
                        {
                            _visualizerWarned = true;
                            LogView.AppendLog("[3D] ❌ Visualizer3D == null! Проверь x:Name в MainWindow.xaml");
                        }
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
                    PidTuner.UpdateFromPacket(packet);
                });

                _telemetryLogger.LogPacket(packet);
            }
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