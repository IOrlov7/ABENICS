// File: HMI_Client/Visual/PidTunerView.xaml.cs
// Code-behind ПИД-тюнера: ползунки -> PID-команды, график отклика каскада.
using System;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using HMI_Client.Comms;
using HMI_Client.Comms.Data;
using LiveChartsCore;
using LiveChartsCore.SkiaSharpView;
using LiveChartsCore.SkiaSharpView.Painting;
using SkiaSharp;

namespace HMI_Client.Visual
{
    public partial class PidTunerView : UserControl
    {
        private CommandDispatcher? _commandDispatcher;

        private readonly ObservableCollection<double> _targetX = new ObservableCollection<double>();
        private readonly ObservableCollection<double> _measuredX = new ObservableCollection<double>();
        private readonly ObservableCollection<double> _encoderX = new ObservableCollection<double>();

        private const int MaxPoints = 300;

        public PidTunerView()
        {
            InitializeComponent();
            InitializeChart();
        }

        public void SetCommandDispatcher(CommandDispatcher dispatcher)
        {
            _commandDispatcher = dispatcher;
        }

        private void InitializeChart()
        {
            PidChart.Series = new ISeries[]
            {
                new LineSeries<double>
                {
                    Name = "Target X", Values = _targetX, Fill = null,
                    Stroke = new SolidColorPaint(SKColors.Red) { StrokeThickness = 2 }
                },
                new LineSeries<double>
                {
                    Name = "IMU Pitch", Values = _measuredX, Fill = null,
                    Stroke = new SolidColorPaint(SKColors.DodgerBlue) { StrokeThickness = 2 }
                },
                new LineSeries<double>
                {
                    Name = "Encoder X", Values = _encoderX, Fill = null,
                    Stroke = new SolidColorPaint(SKColors.LimeGreen) { StrokeThickness = 1 }
                }
            };
        }

        public void UpdateFromPacket(TelemetryPacket packet)
        {
            if (!Dispatcher.CheckAccess())
            {
                Dispatcher.BeginInvoke(new Action(() => UpdateFromPacket(packet)));
                return;
            }

            AddPoint(_targetX, packet.TargetAngleX);
            AddPoint(_measuredX, packet.EulerPitch);
            AddPoint(_encoderX, packet.StepperX_Angle);
        }

        private static void AddPoint(ObservableCollection<double> collection, double value)
        {
            collection.Add(value);
            if (collection.Count > MaxPoints)
                collection.RemoveAt(0);
        }

        private void BtnSendAll_Click(object sender, RoutedEventArgs e)
        {
            var dispatcher = _commandDispatcher;
            if (dispatcher == null)
            {
                MessageBox.Show("Нет активного интерфейса. Сначала подключитесь к ESP32.",
                                "PID Тюнер", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            dispatcher.SendPidCoeffs("X", isOuter: true,
                new PIDCoeffs((float)Sl_OX_Kp.Value, (float)Sl_OX_Ki.Value, (float)Sl_OX_Kd.Value, 0f));

            dispatcher.SendPidCoeffs("X", isOuter: false,
                new PIDCoeffs((float)Sl_IX_Kp.Value, (float)Sl_IX_Ki.Value,
                              (float)Sl_IX_Kd.Value, (float)Sl_IX_Kff.Value));

            OnLog?.Invoke("PID: коэффициенты X отправлены (OX, IX)");
        }

        private void BtnApplyTarget_Click(object sender, RoutedEventArgs e)
        {
            var dispatcher = _commandDispatcher;
            if (dispatcher == null)
            {
                MessageBox.Show("Нет активного интерфейса. Сначала подключитесь к ESP32.",
                                "PID Тюнер", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            var invariant = CultureInfo.InvariantCulture;
            if (!float.TryParse(TxtPitch.Text, NumberStyles.Float, invariant, out float pitch) ||
                !float.TryParse(TxtRoll.Text, NumberStyles.Float, invariant, out float roll))
            {
                MessageBox.Show("Введите числовые значения Pitch и Roll.",
                                "PID Тюнер", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            dispatcher.SendPidTarget(pitch, roll);
            OnLog?.Invoke($"PID: цель установлена pitch={pitch:F2}, roll={roll:F2}");
        }

        public event Action<string>? OnLog;
    }
}