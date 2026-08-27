// File: HMI_Client/Utils/Crc16Helper.cs
//
// НАЗНАЧЕНИЕ:
// Вычисление CRC16, битово совместимое с прошивкой ESP32 (esp_crc16_le из эсп-нулевого ROM).
// Полином: 0x8408 (отражённый 0x1021). См. детали в комментарии к Calculate().
//
using System;

namespace HMI_Client.Utils
{
    public static class Crc16Helper
    {
        /// <summary>
        /// Вычисляет CRC16, повторяя поведение ESP32 ROM crc16_le() при seed = UINT16_MAX.
        /// Возвращает битовый комплемент отражённого CRC-16/CCITT (полином 0x8408) с init = 0x0000.
        /// </summary>
        public static ushort Calculate(byte[] data, int length)
        {
            // Вычисление CRC16, точно совпадающее с ESP32 esp_crc16_le() / crc16_le() из ROM.
            //
            // По SDK "esp_rom/include/esp32/rom/crc.h" ROM-функция crc16_le(seed, buf, len)
            // внутри обёрнута операцией "~" ДО и ПОСЛЕ алгоритма, поэтому для стандартного
            // CRC-16/CCITT-reflected применяется:
            //     crc = ~crc16_le((uint16_t)(~init), buf, length)
            //
            // Прошивка вызывает esp_crc16_le(UINT16_MAX, data, len), т.е. seed = 0xFFFF,
            // что даёт результат = битовый комплемент отражённого CRC-16 с init = ~0xFFFF = 0x0000
            // и полиномом 0x8408 (отражённый 0x1021).
            //
            // ИТОГ: считаем отражённый CRC с начальным регистром 0x0000 и инвертируем результат.
            ushort crc = 0x0000;

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

            return (ushort)~crc;
        }
    }
}