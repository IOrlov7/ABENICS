// File: HMI_Client/App.xaml.cs
using System;
using System.Windows;
using System.Windows.Threading;
using HMI_Client.Utils;

namespace HMI_Client
{
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            // Глобальный отлов необработанных исключений в UI потоке
            this.DispatcherUnhandledException += App_DispatcherUnhandledException;
            // Глобальный отлов исключений в фоновых потоках
            AppDomain.CurrentDomain.UnhandledException += CurrentDomain_UnhandledException;
            
            // Инициализируем UiDispatcher
            UiDispatcher.Initialize();

            base.OnStartup(e);
        }

        private void App_DispatcherUnhandledException(object sender, DispatcherUnhandledExceptionEventArgs e)
        {
            MessageBox.Show($"Критическая ошибка UI:\n\n{e.Exception.Message}\n\n{e.Exception.StackTrace}", 
                            "Ошибка приложения", MessageBoxButton.OK, MessageBoxImage.Error);
            e.Handled = true; // Предотвращаем мгновенный краш, чтобы успеть прочитать
        }

        private void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            if (e.ExceptionObject is Exception ex)
            {
                MessageBox.Show($"Критическая ошибка фонового потока:\n\n{ex.Message}\n\n{ex.StackTrace}", 
                                "Ошибка приложения", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
    }
}