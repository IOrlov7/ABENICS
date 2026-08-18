// File: HMI_Client/Visual/LogView.xaml.cs

using System.Windows.Controls;

namespace HMI_Client.Visual
{
    public partial class LogView : UserControl
    {
        public LogView()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Добавляет сообщение в лог.
        /// </summary>
        /// <param name="message">Сообщение.</param>
        public void AppendLog(string message)
        {
            // Проверяем, вызывается ли метод из UI-потока
            if (this.Dispatcher.CheckAccess())
            {
                // Прямой вызов, если уже в UI-потоке
                TxtLog.AppendText(message + "\n");
                TxtLog.ScrollToEnd();
            }
            else
            {
                // Безопасный вызов из другого потока через Dispatcher
                this.Dispatcher.BeginInvoke(new System.Action(() =>
                {
                    TxtLog.AppendText(message + "\n");
                    TxtLog.ScrollToEnd();
                }));
            }
        }

        /// <summary>
        /// Очищает лог.
        /// </summary>
        public void ClearLog()
        {
            if (this.Dispatcher.CheckAccess())
            {
                TxtLog.Clear();
            }
            else
            {
                this.Dispatcher.BeginInvoke(new System.Action(() => TxtLog.Clear()));
            }
        }
    }
}