using System;
using System.IO.Ports;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using HMI_Client.Comms;

namespace HMI_Client.Visual
{
    public partial class ControlPanelView : UserControl
    {
        private CommandDispatcher? _commandDispatcher;
        private ICommInterface? _currentInterface;

        public event Action<ICommInterface>? OnInterfaceSelected;

        public ControlPanelView()
        {
            InitializeComponent();

            // Подписки на кнопки управления
            BtnConnectUdp.Click += BtnConnectUdp_Click;
            BtnConnectCom.Click += BtnConnectCom_Click;
            BtnRefreshPorts.Click += BtnRefreshPorts_Click;
            BtnEStop.Click += BtnEStop_Click;
            BtnHome.Click += BtnHome_Click;
            BtnCalibrate.Click += BtnCalibrate_Click;
            BtnResume.Click += BtnResume_Click;

            // Инициализация выпадающих списков
            InitializeComPortsList();
            InitializeBaudRatesList();
        }

        public void SetCommandDispatcher(CommandDispatcher dispatcher) => _commandDispatcher = dispatcher;

        // ============================================================
        //  Инициализация ComboBox
        // ============================================================

        private void InitializeComPortsList()
        {
            try
            {
                var ports = SerialPort.GetPortNames().OrderBy(p => p).ToList();
                CmbComPort.ItemsSource = ports;

                if (ports.Count > 0)
                {
                    // Приоритет: COM7 > COM3 > первый доступный
                    var preferred = ports.FirstOrDefault(p => p == "COM7")
                                 ?? ports.FirstOrDefault(p => p == "COM3")
                                 ?? ports[0];
                    CmbComPort.SelectedItem = preferred;
                }
                else
                {
                    CmbComPort.Items.Add("Нет портов");
                    CmbComPort.SelectedIndex = 0;
                    CmbComPort.IsEnabled = false;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Ошибка получения списка портов: {ex.Message}", "Ошибка",
                                MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void InitializeBaudRatesList()
        {
            int[] standardRates = { 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600 };
            CmbBaudRate.ItemsSource = standardRates;
            CmbBaudRate.SelectedItem = 115200; // Дефолтная скорость
        }

        private void BtnRefreshPorts_Click(object sender, RoutedEventArgs e)
        {
            CmbComPort.IsEnabled = true;
            InitializeComPortsList();
        }

        // ============================================================
        //  Подключение по Wi-Fi (без изменений)
        // ============================================================

        private async void BtnConnectUdp_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrWhiteSpace(TxtEspIp.Text))
            {
                MessageBox.Show("Введите IP-адрес ESP32", "Ошибка",
                                MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            try
            {
                var udpInterface = new UdpSender();
                if (await udpInterface.ConnectAsync(TxtEspIp.Text))
                {
                    _currentInterface = udpInterface;
                    OnInterfaceSelected?.Invoke(udpInterface);
                    _commandDispatcher?.SetInterface(udpInterface);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Ошибка подключения по Wi-Fi: {ex.Message}", "Ошибка",
                                MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        // ============================================================
        //  Подключение по COM (с выбором порта и скорости)
        // ============================================================

        private async void BtnConnectCom_Click(object sender, RoutedEventArgs e)
        {
            // Проверка выбора порта
            if (CmbComPort.SelectedItem == null || CmbComPort.SelectedItem.ToString() == "Нет портов")
            {
                MessageBox.Show("Выберите COM-порт из списка", "Ошибка",
                                MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            // Проверка выбора скорости
            if (CmbBaudRate.SelectedItem == null)
            {
                MessageBox.Show("Выберите скорость подключения", "Ошибка",
                                MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            string comPortName = CmbComPort.SelectedItem.ToString()!;
            int baudRate = (int)CmbBaudRate.SelectedItem;

            try
            {
                var serialInterface = new SerialSender();
                
                // Попытка подключения с передачей имени порта и скорости
                // Если ваш SerialSender поддерживает ConnectAsync(string, int) — используем его
                // Иначе используем ConnectAsync(string) и передаем baudRate отдельно
                bool connected = false;
                
                // Проверяем наличие метода с двумя параметрами через reflection
                var methodInfo = typeof(SerialSender).GetMethod("ConnectAsync", 
                    new Type[] { typeof(string), typeof(int) });
                
                if (methodInfo != null)
                {
                    // Метод ConnectAsync(string, int) существует
                    var task = (System.Threading.Tasks.Task<bool>)methodInfo.Invoke(
                        serialInterface, new object[] { comPortName, baudRate })!;
                    connected = await task;
                }
                else
                {
                    // Fallback: пробуем установить BaudRate через свойство
                    var baudProp = typeof(SerialSender).GetProperty("BaudRate");
                    if (baudProp != null && baudProp.CanWrite)
                    {
                        baudProp.SetValue(serialInterface, baudRate);
                    }
                    connected = await serialInterface.ConnectAsync(comPortName);
                }

                if (connected)
                {
                    _currentInterface = serialInterface;
                    OnInterfaceSelected?.Invoke(serialInterface);
                    _commandDispatcher?.SetInterface(serialInterface);
                }
            }
            catch (UnauthorizedAccessException)
            {
                MessageBox.Show(
                    $"Порт {comPortName} занят другим приложением.\nЗакройте другие программы и попробуйте снова.",
                    "Порт занят", MessageBoxButton.OK, MessageBoxImage.Warning);
            }
            catch (System.IO.IOException)
            {
                MessageBox.Show(
                    $"Порт {comPortName} не существует или был отключён.\nОбновите список портов кнопкой 🔄.",
                    "Порт недоступен", MessageBoxButton.OK, MessageBoxImage.Warning);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Ошибка подключения к {comPortName}: {ex.Message}", "Ошибка",
                                MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        // ============================================================
        //  Кнопки управления (без изменений)
        // ============================================================

        private void BtnEStop_Click(object sender, RoutedEventArgs e) => _commandDispatcher?.SendEStop();
        private void BtnHome_Click(object sender, RoutedEventArgs e) => _commandDispatcher?.SendHome();
        private void BtnCalibrate_Click(object sender, RoutedEventArgs e) => _commandDispatcher?.SendCalibrate();
        private void BtnResume_Click(object sender, RoutedEventArgs e) => _commandDispatcher?.SendResume();
    }
}