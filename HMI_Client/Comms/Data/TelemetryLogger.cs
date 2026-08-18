// File: HMI_Client/Data/TelemetryLogger.cs
// Простой класс для логирования телеметрии в CSV-файл. 
// Пока не используется активно, но готов к интеграции.

using System;
using System.IO;
using HMI_Client.Comms.Data;

namespace HMI_Client.Data
{
    public class TelemetryLogger
    {
        private StreamWriter? _writer;
        private readonly string _fileName;
        private bool _isLogging = false;

        public TelemetryLogger(string fileNamePrefix = "telemetry_log")
        {
            _fileName = fileNamePrefix + "_" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".csv";
        }

        public void StartLogging()
        {
            if (_isLogging) return;
            try
            {
                _writer = new StreamWriter(_fileName, false);
                _writer.WriteLine("Timestamp,PacketId,QuatW,QuatX,QuatY,QuatZ,EulerRoll,EulerPitch,EulerYaw,AccelX,AccelY,AccelZ,GyroX,GyroY,GyroZ,MagX,MagY,MagZ,StepperXAngle,StepperYAngle,Servo0,Servo1,Servo2,Servo3,Servo4,Servo5,Servo6,Servo7,WifiRssi,CurrentCmd,StatusFlags");
                _writer.Flush();
                _isLogging = true;
            }
            catch (Exception ex) { Console.WriteLine($"[LOGGER] Ошибка старта: {ex.Message}"); }
        }

        public void LogPacket(TelemetryPacket packet)
        {
            if (!_isLogging || _writer == null) return;
            try
            {
                var line = $"{DateTime.UtcNow:s},{packet.PacketId}," +
                           $"{packet.QuatW:F6},{packet.QuatX:F6},{packet.QuatY:F6},{packet.QuatZ:F6}," +
                           $"{packet.EulerRoll:F6},{packet.EulerPitch:F6},{packet.EulerYaw:F6}," +
                           $"{packet.AccelX:F6},{packet.AccelY:F6},{packet.AccelZ:F6}," +
                           $"{packet.GyroX:F6},{packet.GyroY:F6},{packet.GyroZ:F6}," +
                           $"{packet.MagX:F6},{packet.MagY:F6},{packet.MagZ:F6}," +
                           $"{packet.StepperX_Angle:F6},{packet.StepperY_Angle:F6}," +
                           $"{packet.ServoAngles[0]},{packet.ServoAngles[1]},{packet.ServoAngles[2]},{packet.ServoAngles[3]}," +
                           $"{packet.ServoAngles[4]},{packet.ServoAngles[5]},{packet.ServoAngles[6]},{packet.ServoAngles[7]}," +
                           $"{packet.WifiRssi},{(byte)packet.CurrentCmd:X2},{(byte)packet.StatusFlags:X2}";
                _writer.WriteLine(line);
                _writer.Flush();
            }
            catch { }
        }

        public void StopLogging()
        {
            if (!_isLogging) return;
            _writer?.Close();
            _writer = null;
            _isLogging = false;
        }
    }
}