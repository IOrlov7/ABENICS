// File: HMI_Client/Comms/CommandDispatcher.cs
using HMI_Client.Comms.Data;
using System.Text;

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

        // ★ НОВОЕ: Методы для PID-тюнинга
        public void SendPidCoeffs(string axis, bool isOuter, PIDCoeffs coeffs)
        {
            string prefix = isOuter ? "O" : "I";
            string cmd = $"PID:{prefix}{axis}:{coeffs.Kp:F3},{coeffs.Ki:F3},{coeffs.Kd:F3},{coeffs.Kff:F3}";
            SendTextCommand(cmd);
        }

        public void SendPidTarget(float pitch, float roll)
        {
            string cmd = $"PID:TGT:{pitch:F2},{roll:F2}";
            SendTextCommand(cmd);
        }

        private void SendTextCommand(string text)
        {
            // Отправка текстовой команды через активный интерфейс
            // Предполагается, что ICommInterface имеет метод SendTextCommand
            // Если нет, нужно добавить его в интерфейс
            byte[] data = Encoding.UTF8.GetBytes(text);
            _currentInterface?.SendRawData(data);
        }
    }

    // ★ НОВОЕ: Структура для хранения коэффициентов PID
    public record PIDCoeffs(float Kp, float Ki, float Kd, float Kff);
}