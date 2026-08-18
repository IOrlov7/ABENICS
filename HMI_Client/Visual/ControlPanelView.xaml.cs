using System;
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
            BtnConnectUdp.Click += BtnConnectUdp_Click;
            BtnConnectCom.Click += BtnConnectCom_Click;
            BtnEStop.Click += BtnEStop_Click;
            BtnHome.Click += BtnHome_Click;
            BtnCalibrate.Click += BtnCalibrate_Click;
            BtnResume.Click += BtnResume_Click;
        }

        public void SetCommandDispatcher(CommandDispatcher dispatcher) => _commandDispatcher = dispatcher;

        private async void BtnConnectUdp_Click(object sender, RoutedEventArgs e)
        {
            var udpInterface = new UdpSender();
            if (await udpInterface.ConnectAsync(TxtEspIp.Text))
            {
                _currentInterface = udpInterface;
                OnInterfaceSelected?.Invoke(udpInterface);
                _commandDispatcher?.SetInterface(udpInterface);
            }
        }

        private async void BtnConnectCom_Click(object sender, RoutedEventArgs e)
        {
            string comPortName = "COM3"; // Замените на выбор из UI
            var serialInterface = new SerialSender();
            if (await serialInterface.ConnectAsync(comPortName))
            {
                _currentInterface = serialInterface;
                OnInterfaceSelected?.Invoke(serialInterface);
                _commandDispatcher?.SetInterface(serialInterface);
            }
        }

        private void BtnEStop_Click(object sender, RoutedEventArgs e) => _commandDispatcher?.SendEStop();
        private void BtnHome_Click(object sender, RoutedEventArgs e) => _commandDispatcher?.SendHome();
        private void BtnCalibrate_Click(object sender, RoutedEventArgs e) => _commandDispatcher?.SendCalibrate();
        private void BtnResume_Click(object sender, RoutedEventArgs e) => _commandDispatcher?.SendResume();
    }
}