// File: HMI_Client/Utils/Crc16Helper.cs
//
// НАЗНАЧЕНИЕ:
// Вычисление CRC16 по алгоритму ESP-IDF (esp_crc16_le).
// Это CRC-16/CCITT-FALSE: полином 0x1021, init 0xFFFF, без отражения.
//
using System;

namespace HMI_Client.Utils
{
    public static class Crc16Helper
    {
        /// <summary>
        /// Вычисляет CRC16 по алгоритму CRC-16/CCITT-FALSE.
        /// Полином: 0x1021, начальное значение: 0xFFFF.
        /// </summary>
        public static ushort Calculate(byte[] data, int length)
        {
            ushort crc = 0xFFFF;
            
            for (int i = 0; i < length; i++)
            {
                crc ^= (ushort)(data[i] << 8);
                
                for (int j = 0; j < 8; j++)
                {
                    if ((crc & 0x8000) != 0)
                    {
                        crc = (ushort)((crc << 1) ^ 0x1021);
                    }
                    else
                    {
                        crc <<= 1;
                    }
                }
            }
            
            return crc;
        }
    }
}