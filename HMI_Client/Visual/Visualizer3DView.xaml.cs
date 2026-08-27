// File: HMI_Client/Visual/Visualizer3DView.xaml.cs
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

        public void UpdateRotation(double w, double x, double y, double z)
        {
            _updateCount++;

            // 🔴 Логируем первые 5 вызовов
            if (_updateCount <= 5)
            {
                Debug.WriteLine($"[3D] UpdateRotation #{_updateCount}: W={w:F4}, X={x:F4}, Y={y:F4}, Z={z:F4}");
            }

            if (this.Dispatcher.CheckAccess())
            {
                if (ImuRotation == null)
                {
                    Debug.WriteLine("[3D] ❌ ImuRotation == null!");
                    return;
                }

                try
                {
                    var quaternion = new Quaternion(x, y, z, w);
                    ImuRotation.Rotation = new QuaternionRotation3D(quaternion);
                    
                    if (_updateCount <= 5)
                    {
                        Debug.WriteLine($"[3D] ✅ Transform применён. Angle={quaternion.Angle:F2}°");
                    }
                }
                catch (Exception ex)
                {
                    Debug.WriteLine($"[3D] ❌ Ошибка: {ex.Message}");
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