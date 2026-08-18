// File: HMI_Client/Visual/Visualizer3DView.xaml.cs
// Логика для обновления поворота 3D-модели.

using System.Windows.Controls;
using System.Windows.Media.Media3D;

namespace HMI_Client.Visual
{
    public partial class Visualizer3DView : UserControl
    {
        public Visualizer3DView()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Обновляет поворот 3D-модели по кватерниону.
        /// </summary>
        /// <param name="w">Компонента W кватерниона.</param>
        /// <param name="x">Компонента X кватерниона.</param>
        /// <param name="y">Компонента Y кватерниона.</param>
        /// <param name="z">Компонента Z кватерниона.</param>
        public void UpdateRotation(double w, double x, double y, double z)
        {
            // Проверяем, вызывается ли метод из UI-потока
            if (this.Dispatcher.CheckAccess())
            {
                // Прямой вызов, если уже в UI-потоке
                var quaternion = new Quaternion(x, y, z, w); // WPF: X, Y, Z, W
                ImuRotation.Rotation = new QuaternionRotation3D(quaternion);
            }
            else
            {
                // Безопасный вызов из другого потока через Dispatcher
                this.Dispatcher.BeginInvoke(new System.Action(() =>
                {
                    UpdateRotation(w, x, y, z); // Рекурсивный вызов в UI-потоке
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
                ImuRotation.Rotation = new QuaternionRotation3D(new Quaternion(0, 0, 0, 1));
            }
            else
            {
                this.Dispatcher.BeginInvoke(new System.Action(() => ResetRotation()));
            }
        }
    }
}