// File: HMI_Client/Visual/TelemetryGraphsView.xaml.cs
// Логика для обновления графиков. Использует ObservableCollection<double> и ISeries из LiveCharts.

using System;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Controls;
using LiveChartsCore;
using LiveChartsCore.SkiaSharpView;
using LiveChartsCore.SkiaSharpView.Painting;
using SkiaSharp;

namespace HMI_Client.Visual
{
    public partial class TelemetryGraphsView : UserControl
    {
        // Коллекции для данных графиков
        private readonly ObservableCollection<double> _accelX = new ObservableCollection<double>();
        private readonly ObservableCollection<double> _accelY = new ObservableCollection<double>();
        private readonly ObservableCollection<double> _accelZ = new ObservableCollection<double>();
        private readonly ObservableCollection<double> _gyroX = new ObservableCollection<double>();
        private readonly ObservableCollection<double> _gyroY = new ObservableCollection<double>();
        private readonly ObservableCollection<double> _gyroZ = new ObservableCollection<double>();

        private const int MaxPoints = 100; // Максимальное количество точек на графике
        private int _currentIndex = 0; // Индекс времени для оси X

        public TelemetryGraphsView()
        {
            InitializeComponent();
            InitializeCharts();
        }

        private void InitializeCharts()
        {
            // Настройка графика акселерометра
            AccelChart.Series = new ISeries[]
            {
                new LineSeries<double> { Name = "X", Values = _accelX, Fill = null, Stroke = new SolidColorPaint(SKColors.Blue) { StrokeThickness = 2 } },
                new LineSeries<double> { Name = "Y", Values = _accelY, Fill = null, Stroke = new SolidColorPaint(SKColors.Green) { StrokeThickness = 2 } },
                new LineSeries<double> { Name = "Z", Values = _accelZ, Fill = null, Stroke = new SolidColorPaint(SKColors.Red) { StrokeThickness = 2 } }
            };

            // Настройка графика гироскопа
            GyroChart.Series = new ISeries[]
            {
                new LineSeries<double> { Name = "X", Values = _gyroX, Fill = null, Stroke = new SolidColorPaint(SKColors.Blue) { StrokeThickness = 2 } },
                new LineSeries<double> { Name = "Y", Values = _gyroY, Fill = null, Stroke = new SolidColorPaint(SKColors.Green) { StrokeThickness = 2 } },
                new LineSeries<double> { Name = "Z", Values = _gyroZ, Fill = null, Stroke = new SolidColorPaint(SKColors.Red) { StrokeThickness = 2 } }
            };
        }

        /// <summary>
        /// Обновляет данные на графиках.
        /// </summary>
        /// <param name="ax">Ускорение по X.</param>
        /// <param name="ay">Ускорение по Y.</param>
        /// <param name="az">Ускорение по Z.</param>
        /// <param name="gx">Угловая скорость по X.</param>
        /// <param name="gy">Угловая скорость по Y.</param>
        /// <param name="gz">Угловая скорость по Z.</param>
        public void UpdateCharts(double ax, double ay, double az, double gx, double gy, double gz)
        {
            // Проверяем, вызывается ли метод из UI-потока
            if (this.Dispatcher.CheckAccess())
            {
                // Прямой вызов, если уже в UI-потоке
                _currentIndex++;

                // Добавляем новые точки
                _accelX.Add(ax);
                _accelY.Add(ay);
                _accelZ.Add(az);
                _gyroX.Add(gx);
                _gyroY.Add(gy);
                _gyroZ.Add(gz);

                // Удаляем старые точки, если превышен лимит
                if (_accelX.Count > MaxPoints) _accelX.RemoveAt(0);
                if (_accelY.Count > MaxPoints) _accelY.RemoveAt(0);
                if (_accelZ.Count > MaxPoints) _accelZ.RemoveAt(0);
                if (_gyroX.Count > MaxPoints) _gyroX.RemoveAt(0);
                if (_gyroY.Count > MaxPoints) _gyroY.RemoveAt(0);
                if (_gyroZ.Count > MaxPoints) _gyroZ.RemoveAt(0);
            }
            else
            {
                // Безопасный вызов из другого потока через Dispatcher
                this.Dispatcher.BeginInvoke(new Action(() =>
                {
                    UpdateCharts(ax, ay, az, gx, gy, gz); // Рекурсивный вызов в UI-потоке
                }));
            }
        }

        /// <summary>
        /// Очищает все графики.
        /// </summary>
        public void ClearCharts()
        {
            if (this.Dispatcher.CheckAccess())
            {
                _accelX.Clear();
                _accelY.Clear();
                _accelZ.Clear();
                _gyroX.Clear();
                _gyroY.Clear();
                _gyroZ.Clear();
                _currentIndex = 0;
            }
            else
            {
                this.Dispatcher.BeginInvoke(new Action(() => ClearCharts()));
            }
        }
    }
}