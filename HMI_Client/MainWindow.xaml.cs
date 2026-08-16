using System;
using System.Globalization;
using System.IO.Ports;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media.Media3D;
using System.Windows.Threading;

namespace IMU_Visualizer
{
    public partial class MainWindow : Window
    {
        // Настройки
        private const int BaudRate = 921600;
        private const double SlerpFactor = 0.35;

        // COM
        private SerialPort? serialPort;
        private string rxBuffer = string.Empty;

        // 3D
        private readonly Transform3DGroup modelTransform = new Transform3DGroup();
        private readonly QuaternionRotation3D modelRotation = new QuaternionRotation3D(Quaternion.Identity);

        // Синхронизация данных
        private readonly object dataLock = new object();
        private Quaternion targetQuaternion = Quaternion.Identity;
        private Quaternion displayQuaternion = Quaternion.Identity;
        private bool hasNewQuaternion;
        private bool hasDisplayQuaternion;
        private double lastRoll, lastPitch, lastYaw, lastTemp;
        private bool hasNewText;
        private int packetCount;
        private string lastPacketTime = "--";

        // Таймеры
        private readonly DispatcherTimer renderTimer = new DispatcherTimer();
        private readonly DispatcherTimer rateTimer = new DispatcherTimer();

        public MainWindow()
        {
            InitializeComponent();
            InitializeSerialPorts();
            Setup3DModel();
            StartTimers();
        }

        private void InitializeSerialPorts()
        {
            try
            {
                string[] ports = SerialPort.GetPortNames();
                Array.Sort(ports);
                CmbPort.ItemsSource = ports;
                if (ports.Length > 0) CmbPort.SelectedIndex = 0;
            }
            catch { CmbPort.ItemsSource = new string[0]; }
        }

        private void Setup3DModel()
        {
            modelTransform.Children.Add(new RotateTransform3D(modelRotation));
            ImuModel.Transform = modelTransform;
        }

        private void StartTimers()
        {
            renderTimer.Interval = TimeSpan.FromMilliseconds(33);
            renderTimer.Tick += RenderTimer_Tick;
            renderTimer.Start();

            rateTimer.Interval = TimeSpan.FromSeconds(1);
            rateTimer.Tick += RateTimer_Tick;
            rateTimer.Start();
        }

        private void RenderTimer_Tick(object sender, EventArgs e)
        {
            lock (dataLock)
            {
                if (hasNewQuaternion)
                {
                    if (!hasDisplayQuaternion)
                    {
                        displayQuaternion = targetQuaternion;
                        hasDisplayQuaternion = true;
                    }
                    else
                    {
                        targetQuaternion = MakeShortestPath(displayQuaternion, targetQuaternion);
                        displayQuaternion = Quaternion.Slerp(displayQuaternion, targetQuaternion, SlerpFactor);
                    }
                    displayQuaternion.Normalize();
                    modelRotation.Quaternion = displayQuaternion;
                    hasNewQuaternion = false;
                }

                if (hasNewText)
                {
                    TxtRoll.Text = lastRoll.ToString("F1", CultureInfo.InvariantCulture) + "°";
                    TxtPitch.Text = lastPitch.ToString("F1", CultureInfo.InvariantCulture) + "°";
                    TxtYaw.Text = lastYaw.ToString("F1", CultureInfo.InvariantCulture) + "°";
                    TxtTemp.Text = lastTemp.ToString("F1", CultureInfo.InvariantCulture) + " °C";
                    hasNewText = false;
                }
            }

            TxtLastPacket.Text = $"Последний пакет: {lastPacketTime}";
        }

        private void RateTimer_Tick(object sender, EventArgs e)
        {
            int count = Interlocked.Exchange(ref packetCount, 0);
            TxtPacketRate.Text = $"{count} Hz";

            var port = serialPort;
            if (port != null && port.IsOpen)
            {
                TxtStatus.Text = count > 0 ? "🟢 Подключено. Данные идут" : "🟡 Подключено. Нет данных";
                TxtStatus.Foreground = count > 0 ? System.Windows.Media.Brushes.Green : System.Windows.Media.Brushes.Orange;
            }
            else
            {
                TxtStatus.Text = "⚪ Отключено";
                TxtStatus.Foreground = System.Windows.Media.Brushes.Gray;
            }
        }

        private static Quaternion MakeShortestPath(Quaternion from, Quaternion to)
        {
            double dot = from.X * to.X + from.Y * to.Y + from.Z * to.Z + from.W * to.W;
            if (dot < 0) return new Quaternion(-to.X, -to.Y, -to.Z, -to.W);
            return to;
        }

        private static Quaternion ConvertSensorQuaternionToWpf(double qw, double qx, double qy, double qz)
        {
            // FLU (Front-Left-Up) -> WPF (Right-Up-Towards)
            // WPF_X = -FLU_Y
            // WPF_Y =  FLU_Z
            // WPF_Z =  FLU_X
            return new Quaternion(-qy, qz, qx, qw);
        }

        private void BtnComOpen_Click(object sender, RoutedEventArgs e)
        {
            if (CmbPort.SelectedItem == null) return;

            string portName = CmbPort.SelectedItem.ToString()!;
            if (serialPort != null && serialPort.IsOpen) return;

            try
            {
                var port = new SerialPort(portName, BaudRate)
                {
                    ReadTimeout = 200,
                    WriteTimeout = 200
                };
                port.DataReceived += SerialPort_DataReceived;
                port.Open();
                serialPort = port;
                rxBuffer = string.Empty;
                TxtStatus.Text = "🟢 Подключено";
                TxtStatus.Foreground = System.Windows.Media.Brushes.Green;
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Ошибка: {ex.Message}", "Ошибка", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void BtnComClose_Click(object sender, RoutedEventArgs e)
        {
            var port = serialPort;
            serialPort = null;

            if (port == null) return;

            port.DataReceived -= SerialPort_DataReceived;
            Task.Run(() =>
            {
                try { if (port.IsOpen) port.Close(); } catch { }
                try { port.Dispose(); } catch { }
            });

            TxtStatus.Text = "⚪ Отключено";
            TxtStatus.Foreground = System.Windows.Media.Brushes.Gray;
        }

        private void SerialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            var port = sender as SerialPort;
            if (port == null) return;

            try
            {
                string incoming = port.ReadExisting();
                rxBuffer += incoming;

                while (true)
                {
                    int idx = rxBuffer.IndexOf('\n');
                    if (idx < 0) break;

                    string line = rxBuffer.Substring(0, idx).TrimEnd('\r');
                    rxBuffer = rxBuffer.Substring(idx + 1);

                    if (line.StartsWith("DATA|"))
                    {
                        ProcessDataLine(line);
                        Interlocked.Increment(ref packetCount);
                    }
                    else
                    {
                        Dispatcher.BeginInvoke(() => AppendLog(line));
                    }
                }
            }
            catch { /* ignore */ }
        }

        private void ProcessDataLine(string line)
        {
            try
            {
                string[] parts = line.Split('|');
                double qw = 0, qx = 0, qy = 0, qz = 0, temp = 0, roll = 0, pitch = 0, yaw = 0;

                bool hasQw = false, hasQx = false, hasQy = false, hasQz = false;
                bool hasTemp = false, hasRoll = false, hasPitch = false, hasYaw = false;

                foreach (string part in parts)
                {
                    if (TryParseValue(part, "T:", out double t)) { temp = t; hasTemp = true; }
                    if (TryParseValue(part, "R:", out double r)) { roll = r; hasRoll = true; }
                    if (TryParseValue(part, "P:", out double p)) { pitch = p; hasPitch = true; }
                    if (TryParseValue(part, "Y:", out double y1)) { yaw = y1; hasYaw = true; } // renamed y to y1
                    if (TryParseValue(part, "QW:", out double w)) { qw = w; hasQw = true; }
                    if (TryParseValue(part, "QX:", out double x)) { qx = x; hasQx = true; }
                    if (TryParseValue(part, "QY:", out double y2)) { qy = y2; hasQy = true; } // renamed y to y2
                    if (TryParseValue(part, "QZ:", out double z)) { qz = z; hasQz = true; }
                }

                if (hasQw && hasQx && hasQy && hasQz)
                {
                    double len = Math.Sqrt(qw * qw + qx * qx + qy * qy + qz * qz);
                    if (len > 0.001)
                    {
                        Quaternion sensorQuat = ConvertSensorQuaternionToWpf(qw, qx, qy, qz);
                        sensorQuat.Normalize();

                        lock (dataLock)
                        {
                            targetQuaternion = sensorQuat;
                            hasNewQuaternion = true;
                        }
                    }
                }

                if (hasTemp || hasRoll || hasPitch || hasYaw)
                {
                    lock (dataLock)
                    {
                        if (hasRoll) lastRoll = roll;
                        if (hasPitch) lastPitch = pitch;
                        if (hasYaw) lastYaw = yaw;
                        if (hasTemp) lastTemp = temp;
                        hasNewText = true;
                    }
                }

                lastPacketTime = DateTime.Now.ToString("HH:mm:ss.fff");
            }
            catch { /* ignore */ }
        }

        private static bool TryParseValue(string part, string key, out double value)
        {
            value = 0;
            if (!part.StartsWith(key, StringComparison.Ordinal)) return false;
            string text = part.Substring(key.Length);
            return double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out value);
        }

        private void AppendLog(string message)
        {
            if (TxtLog.Text.Length > 200000) TxtLog.Clear();
            TxtLog.AppendText($"{DateTime.Now:HH:mm:ss} | {message}{Environment.NewLine}");
            TxtLog.ScrollToEnd();
        }

        private void SendCommand(string command)
        {
            var port = serialPort;
            if (port == null || !port.IsOpen) return;

            try { port.WriteLine(command); AppendLog($"Отправлено: {command}"); }
            catch (Exception ex) { AppendLog($"Ошибка: {ex.Message}"); }
        }

        private void BtnMode0_Click(object sender, RoutedEventArgs e) => SendCommand("MODE 0");
        private void BtnMode1_Click(object sender, RoutedEventArgs e) => SendCommand("MODE 1");
        private void BtnMode2_Click(object sender, RoutedEventArgs e) => SendCommand("MODE 2");
        private void BtnSendIpToEsp_Click(object sender, RoutedEventArgs e) => SendCommand($"IP {TxtPcIp.Text}");
        private void BtnSave_Click(object sender, RoutedEventArgs e) => SendCommand("SAVE");
        private void BtnReset_Click(object sender, RoutedEventArgs e) => SendCommand("RESET");
        private void BtnRefreshPorts_Click(object sender, RoutedEventArgs e) => InitializeSerialPorts();

        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            renderTimer.Stop();
            rateTimer.Stop();

            var port = serialPort;
            serialPort = null;

            if (port != null && port.IsOpen)
            {
                port.DataReceived -= SerialPort_DataReceived;
                Task.Run(() =>
                {
                    try { if (port.IsOpen) port.Close(); } catch { }
                    try { port.Dispose(); } catch { }
                });
            }

            base.OnClosing(e);
        }
    }
}