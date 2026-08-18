// File: HMI_Client/Comms/CommandDispatcher.cs
//
// НАЗНАЧЕНИЕ:
// Центральный узел для отправки команд.
// Хранит ссылку на активный ICommInterface и перенаправляет команды (E-STOP, HOME и т.д.) ему.
// Это позволяет UI-компонентам (например, кнопкам) вызывать команды, не зная, какое именно соединение (UDP, Serial) используется.
//
// ОТВЕЧАЕТ ЗА:
// - Хранение активного интерфейса связи.
// - Маршрутизацию команд к активному интерфейсу.

using HMI_Client.Comms.Data;

namespace HMI_Client.Comms
{
    public class CommandDispatcher
    {
        private ICommInterface? _currentInterface;

        public void SetInterface(ICommInterface commInterface)
        {
            _currentInterface = commInterface;
        }

        public void SendCommand(CommandId cmd, byte[]? payload = null)
        {
            _currentInterface?.SendCommand(cmd, payload);
        }

        public void SendEStop() => SendCommand(CommandId.CMD_ESTOP);
        public void SendHome() => SendCommand(CommandId.CMD_HOME);
        public void SendCalibrate() => SendCommand(CommandId.CMD_CALIBRATE);
        public void SendResume() => SendCommand(CommandId.CMD_RESUME);
    }
}