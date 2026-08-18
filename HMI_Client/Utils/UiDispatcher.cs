using System;
using System.Windows;
using System.Windows.Threading;

namespace HMI_Client.Utils
{
    public static class UiDispatcher
    {
        private static Dispatcher? _uiDispatcher;

        public static void Initialize()
        {
            _uiDispatcher = Application.Current?.Dispatcher ?? Dispatcher.CurrentDispatcher;
        }

        public static void Invoke(Action action)
        {
            if (_uiDispatcher != null && !_uiDispatcher.HasShutdownStarted)
            {
                if (_uiDispatcher.CheckAccess()) action();
                else _uiDispatcher.Invoke(action);
            }
        }
    }
}