// File: HMI_Client/Visual/Visualizer3DView.xaml.cs
// Логика для обновления поворота 3D-модели.
using System;
using System.Diagnostics;
using System.Windows.Controls;
using System.Windows.Media.Media3D;

namespace HMI_Client.Visual
{
    public partial class Visualizer3DView : UserControl
    {
        private int _updateCount = 0;

        public Visualizer3DView()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Обновляет поворот 3D-модели по кватерниону.
        /// </summary>
        public void UpdateRotation(double w, double x, double y, double z)
        {
            _updateCount++;

            // 🔴 ДИАГНОСТИКА: Логируем первый вызов и каждый 50-й
            if (_updateCount == 1)
            {
                Debug.WriteLine("[3D] ✅ UpdateRotation вызван ВПЕРВЫЕ!");
            }
            else if (_updateCount % 50 == 0)
            {
                double norm = Math.Sqrt(w * w + x * x + y * y + z * z);
                Debug.WriteLine($"[3D] 🔄 UpdateRotation #{_updateCount}: W={w:F4}, X={x:F4}, Y={y:F4}, Z={z:F4}, Norm={norm:F4}");
            }

            if (this.Dispatcher.CheckAccess())
            {
                if (ImuRotation == null)
                {
                    Debug.WriteLine("[3D] ❌ ImuRotation == null! Проверь x:Name=\"ImuRotation\" в XAML");
                    return;
                }

                try
                {
                    // 🔴 Мягкая нормализация
                    double norm = Math.Sqrt(w * w + x * x + y * y + z * z);
                    if (norm > 0.001 && Math.Abs(norm - 1.0) > 0.1)
                    {
                        w /= norm; x /= norm; y /= norm; z /= norm;
                        if (_updateCount % 50 == 0) Debug.WriteLine("[3D] ⚠️ Кватернион принудительно нормализован");
                    }

                    // WPF Quaternion: X, Y, Z, W
                    var quaternion = new Quaternion(x, y, z, w);
                    var rotation = new QuaternionRotation3D(quaternion);
                    ImuRotation.Rotation = rotation;

                    // 🔴 ДИАГНОСТИКА: обращаемся к quaternion.Axis и quaternion.Angle (НЕ к rotation)
                    if (_updateCount % 50 == 0)
                    {
                        var axis = quaternion.Axis;
                        Debug.WriteLine($"[3D] ✅ Transform применён. Axis=({axis.X:F2}, {axis.Y:F2}, {axis.Z:F2}), Angle={quaternion.Angle:F2}°");
                    }
                }
                catch (Exception ex)
                {
                    Debug.WriteLine($"[3D] ❌ Ошибка применения Transform: {ex.Message}");
                }
            }
            else
            {
                this.Dispatcher.BeginInvoke(new Action(() =>
                {
                    UpdateRotation(w, x, y, z);
                }));
            }
        }

        /// <summary>
        /// Сбрасывает поворот модели.
        /// </summary>
        public void ResetRotation()
        {
            if (this.Dispatcher.CheckAccess())
            {
                if (ImuRotation != null)
                {
                    ImuRotation.Rotation = new QuaternionRotation3D(new Quaternion(0, 0, 0, 1));
                }
            }
            else
            {
                this.Dispatcher.BeginInvoke(new Action(() => ResetRotation()));
            }
        }
    }
}