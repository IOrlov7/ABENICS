using System;
using System.Windows;
using System.Windows.Threading;
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
            UiDispatcher.Invoke(() =>
            {
                Visualizer3D.UpdateRotation(packet.QuatW, packet.QuatX, packet.QuatY, packet.QuatZ);
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