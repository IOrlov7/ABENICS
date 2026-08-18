using System;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Media.Media3D;
using System.Windows.Threading;
using HelixToolkit.Wpf;
using LiveChartsCore;
using LiveChartsCore.SkiaSharpView;
using LiveChartsCore.SkiaSharpView.Painting;
using SkiaSharp;
using HMI_Client; // Подключаем пространство имён, где определены TelemetryPacket, NetworkManager, CommandId и т.д.

namespace IMU_Visualizer
{
    public partial class MainWindow : Window
    {
        private NetworkManager _networkManager;
        private TelemetryPacket? _lastPacket; // Теперь nullable

        // Данные для графиков (последние 100 точек)
        private const int MAX_POINTS = 100;
        private ObservableCollection<double> _accelX = new ObservableCollection<double>();
        private ObservableCollection<double> _accelY = new ObservableCollection<double>();
        private ObservableCollection<double> _accelZ = new ObservableCollection<double>();
        private ObservableCollection<double> _gyroX = new ObservableCollection<double>();
        private ObservableCollection<double> _gyroY = new ObservableCollection<double>();
        private ObservableCollection<double> _gyroZ = new ObservableCollection<double>();

        public MainWindow()
        {
            InitializeComponent();

            _networkManager = new NetworkManager();
            _networkManager.OnTelemetryReceived += OnTelemetryReceived;
            _networkManager.OnLogMessage += OnLogMessage;

            InitializeCharts(); // Инициализация графиков LiveCharts

            // Таймер для обновления UI (например, для отображения частоты приёма)
            var uiTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(200) };
            uiTimer.Tick += (s, e) => UpdateUiRateDisplay();
            uiTimer.Start();

            // Запуск сетевого менеджера
            _networkManager.Start();

            // Лог начального состояния
            OnLogMessage("Клиент ABENICS запущен. Ожидание подключения ESP32...");
        }

        protected override void OnClosed(EventArgs e)
        {
            _networkManager.Stop();
            base.OnClosed(e);
        }

        private void InitializeCharts()
        {
            // Настройка графика акселерометра
            AccelChart.Series = new ISeries[]
            {
                new LineSeries<double>
                {
                    Values = _accelX,
                    Name = "X",
                    Stroke = new SolidColorPaint(SKColors.Blue) { StrokeThickness = 2 },
                    GeometryStroke = null,
                    GeometrySize = 0,
                    Fill = null
                },
                new LineSeries<double>
                {
                    Values = _accelY,
                    Name = "Y",
                    Stroke = new SolidColorPaint(SKColors.Green) { StrokeThickness = 2 },
                    GeometryStroke = null,
                    GeometrySize = 0,
                    Fill = null
                },
                new LineSeries<double>
                {
                    Values = _accelZ,
                    Name = "Z",
                    Stroke = new SolidColorPaint(SKColors.Red) { StrokeThickness = 2 },
                    GeometryStroke = null,
                    GeometrySize = 0,
                    Fill = null
                }
            };

            AccelChart.XAxes = new Axis[] { new Axis { Labeler = value => "" } };
            AccelChart.YAxes = new Axis[] { new Axis { Name = "m/s²" } };

            // Настройка графика гироскопа
            GyroChart.Series = new ISeries[]
            {
                new LineSeries<double>
                {
                    Values = _gyroX,
                    Name = "X",
                    Stroke = new SolidColorPaint(SKColors.Blue) { StrokeThickness = 2 },
                    GeometryStroke = null,
                    GeometrySize = 0,
                    Fill = null
                },
                new LineSeries<double>
                {
                    Values = _gyroY,
                    Name = "Y",
                    Stroke = new SolidColorPaint(SKColors.Green) { StrokeThickness = 2 },
                    GeometryStroke = null,
                    GeometrySize = 0,
                    Fill = null
                },
                new LineSeries<double>
                {
                    Values = _gyroZ,
                    Name = "Z",
                    Stroke = new SolidColorPaint(SKColors.Red) { StrokeThickness = 2 },
                    GeometryStroke = null,
                    GeometrySize = 0,
                    Fill = null
                }
            };

            GyroChart.XAxes = new Axis[] { new Axis { Labeler = value => "" } };
            GyroChart.YAxes = new Axis[] { new Axis { Name = "°/s" } };
        }

        // Метод обновления данных графиков (вызывайте при получении нового пакета)
        private void UpdateChartData(float ax, float ay, float az, float gx, float gy, float gz)
        {
            // Добавляем новые точки
            _accelX.Add(ax);
            _accelY.Add(ay);
            _accelZ.Add(az);
            _gyroX.Add(gx);
            _gyroY.Add(gy);
            _gyroZ.Add(gz);

            // Удаляем старые точки, если превышен лимит
            if (_accelX.Count > MAX_POINTS) _accelX.RemoveAt(0);
            if (_accelY.Count > MAX_POINTS) _accelY.RemoveAt(0);
            if (_accelZ.Count > MAX_POINTS) _accelZ.RemoveAt(0);
            if (_gyroX.Count > MAX_POINTS) _gyroX.RemoveAt(0);
            if (_gyroY.Count > MAX_POINTS) _gyroY.RemoveAt(0);
            if (_gyroZ.Count > MAX_POINTS) _gyroZ.RemoveAt(0);
        }

        private void OnTelemetryReceived(TelemetryPacket packet)
        {
            _lastPacket = packet; // Сохраняем как nullable

            // Обновление UI должно происходить в главном потоке
            Dispatcher.Invoke(() =>
            {
                // 1. Статус
                TxtStatus.Text = $"🟢 Подключено к {_networkManager.GetEsp32Ip()}";
                TxtPacketRate.Text = $"~{packet.TimestampMs / 1000:F1} Hz"; // Грубая оценка частоты
                TxtLastPacket.Text = $"ID: {packet.PacketId}, RSSI: {packet.WifiRssi}dBm, CMD: {packet.CurrentCmd}";

                // 2. Флаги (выводим как есть, без обращения к TxtFlags)
                // TxtFlags.Text = $"Flags: 0x{(byte)packet.StatusFlags:X2} ({packet.CurrentCmd})"; // УДАЛЕНА ЭТА СТРОКА

                // 3. IMU Данные
                TxtRoll.Text = $"{packet.EulerRoll:F1}°";
                TxtPitch.Text = $"{packet.EulerPitch:F1}°";
                TxtYaw.Text = $"{packet.EulerYaw:F1}°";
                // TxtTemp.Text = $"-- °C"; // Температура пока не передаётся в пакете

                // 4. Обновление 3D модели по кватерниону
                Update3DModel(packet.QuatW, packet.QuatX, packet.QuatY, packet.QuatZ);

                // 5. Обновление графиков
                UpdateChartData(
                    packet.AccelX, packet.AccelY, packet.AccelZ,
                    packet.GyroX, packet.GyroY, packet.GyroZ
                );
            });
        }

        private void Update3DModel(double w, double x, double y, double z)
        {
            // Преобразование кватерниона (w, x, y, z) в Matrix3D для Helix Toolkit
            double xx = x * x;
            double yy = y * y;
            double zz = z * z;
            double xy = x * y;
            double xz = x * z;
            double yz = y * z;
            double wx = w * x;
            double wy = w * y;
            double wz = w * z;

            Matrix3D rotationMatrix = new Matrix3D(
                1 - 2 * (yy + zz), 2 * (xy - wz), 2 * (xz + wy), 0,
                2 * (xy + wz), 1 - 2 * (xx + zz), 2 * (yz - wx), 0,
                2 * (xz - wy), 2 * (yz + wx), 1 - 2 * (xx + yy), 0,
                0, 0, 0, 1
            );

            if (ImuModel.Transform is MatrixTransform3D matrixTransform)
            {
                matrixTransform.Matrix = rotationMatrix;
            }
            else
            {
                ImuModel.Transform = new MatrixTransform3D(rotationMatrix);
            }
        }

        private void UpdateUiRateDisplay()
        {
            // Здесь можно обновлять частоту обновления UI, если нужно
            // Проверяем, получен ли хотя бы один пакет
            if (_lastPacket.HasValue) // Проверка на null для nullable struct
            {
                var now = Environment.TickCount & Int32.MaxValue; // millis() аналог
                long delay = now - (int)_lastPacket.Value.TimestampMs; // Используем .Value
                if (delay > 100) // Если пакет старше 100 мс
                {
                    TxtPacketRate.Text = $"⚠️ Delay: {delay}ms";
                }
            }
            else
            {
                TxtPacketRate.Text = "0 Hz"; // Или другой индикатор ожидания
            }
        }

        private void OnLogMessage(string msg)
        {
            TxtLog.AppendText(msg + Environment.NewLine);
            TxtLog.ScrollToEnd();
        }

        // Обработчики кнопок
        private void BtnEStop_Click(object sender, RoutedEventArgs e) => _networkManager.SendEStop();
        private void BtnHome_Click(object sender, RoutedEventArgs e) => _networkManager.SendHome();
        private void BtnCalibrate_Click(object sender, RoutedEventArgs e) => _networkManager.SendCalibrate();
        private void BtnResume_Click(object sender, RoutedEventArgs e) => _networkManager.SendResume();
        private void BtnConnectToAP_Click(object sender, RoutedEventArgs e)
        {
            _networkManager.ConnectToEspManual("192.168.4.1");
        }

        // Заглушка для старых кнопок COM-порта, если они были
        private void BtnComOpen_Click(object sender, RoutedEventArgs e) => OnLogMessage("COM Port: Not used in ABENICS UDP mode.");
        private void BtnComClose_Click(object sender, RoutedEventArgs e) => OnLogMessage("COM Port: Not used in ABENICS UDP mode.");
        private void BtnMode0_Click(object sender, RoutedEventArgs e) => OnLogMessage("Mode 0: Not used in ABENICS UDP mode.");
        private void BtnMode1_Click(object sender, RoutedEventArgs e) => OnLogMessage("Mode 1: Not used in ABENICS UDP mode.");
        private void BtnMode2_Click(object sender, RoutedEventArgs e) => OnLogMessage("Mode 2: Not used in ABENICS UDP mode.");
        private void BtnRefreshPorts_Click(object sender, RoutedEventArgs e) => OnLogMessage("Refresh Ports: Not used in ABENICS UDP mode.");
        private void BtnSendIpToEsp_Click(object sender, RoutedEventArgs e) => OnLogMessage("Send IP to ESP: Not implemented in UDP mode.");
        private void BtnSave_Click(object sender, RoutedEventArgs e) => OnLogMessage("Save Config: Not implemented in UDP mode.");
        private void BtnReset_Click(object sender, RoutedEventArgs e) => OnLogMessage("Reset: Not implemented in UDP mode.");
    }
}