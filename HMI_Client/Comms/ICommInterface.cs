// File: HMI_Client/Comms/ICommInterface.cs
//
// НАЗНАЧЕНИЕ:
// Это ключевой интерфейс в архитектуре ABENICS HMI.
// Он определяет единый контракт для любого источника данных телеметрии (UDP, Serial, и т.д.).
// Позволяет легко переключаться между различными способами подключения без изменения UI или других зависимых компонентов.
//
// ОТВЕЧАЕТ ЗА:
// - Определение общего API для подключения, отключения, отправки команд и получения данных.

// File: HMI_Client/Comms/ICommInterface.cs
using System;
using System.Threading.Tasks;
using HMI_Client.Comms.Data;
using HMI_Client.Comms.Data;

namespace HMI_Client.Comms
{
    public interface ICommInterface
    {
        event Action<TelemetryPacket>? OnTelemetryReceived;
        event Action<string>? OnLogMessage;
        event Action<bool>? OnConnectionChanged;

        // Если строка подключения может быть пустой или null, добавляем '?'
        Task<bool> ConnectAsync(string? connectionString); 
        
        void Disconnect();
        
        // ВАЖНО: byte[]? payload = null (знак вопроса обязателен!)
        void SendCommand(CommandId cmd, byte[]? payload = null); 

        // Отправка произвольных ("сырых") данных — используется для текстовых
        // PID-команд вида "PID:OX:1.5,0.1,0.05,0.0" (без бинарной обёртки AA 55).
        void SendRawData(byte[] data);
    }
}