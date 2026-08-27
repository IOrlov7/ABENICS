// File: HMI_Client/Utils/Crc16Helper.cs
//
// НАЗНАЧЕНИЕ:
// Вычисление CRC16 по алгоритму ESP-IDF (esp_crc16_le).
// Это CRC-16/MODBUS: полином 0x8408 (bit-reversed 0x1021), init 0xFFFF.
//
using System;

namespace HMI_Client.Utils
{
    public static class Crc16Helper
    {
        /// <summary>
        /// Вычисляет CRC16 по алгоритму esp_crc16_le (CRC-16/MODBUS).
        /// Полином: 0x8408 (bit-reversed), начальное значение: 0xFFFF.
        /// Совместим с ESP32 esp_crc16_le(UINT16_MAX, data, len).
        /// </summary>
        public static ushort Calculate(byte[] data, int length)
        {
            ushort crc = 0xFFFF;
            
            for (int i = 0; i < length; i++)
            {
                crc ^= data[i];
                
                for (int j = 0; j < 8; j++)
                {
                    if ((crc & 0x0001) != 0)
                    {
                        crc = (ushort)((crc >> 1) ^ 0x8408);
                    }
                    else
                    {
                        crc >>= 1;
                    }
                }
            }
            
            return crc;
        }
    }
}