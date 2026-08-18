// File: HMI_Client/Utils/Crc16Helper.cs
//
// НАЗНАЧЕНИЕ:
// Этот статический класс предоставляет функцию для вычисления CRC16.
// Используется для проверки целостности полученных UDP-пакетов.
//
// ОТВЕЧАЕТ ЗА:
// - Вычисление CRC16 по алгоритму, совместимому с ESP-IDF (poly 0x8408, init 0xFFFF).

namespace HMI_Client.Utils
{
    public static class Crc16Helper
    {

        /// Вычисляет CRC16 по алгоритму, используемому в ESP-IDF.
        /// Полином: 0x8408 (reflected CCITT / CRC-16/ISO-HDLC).
        /// Начальное значение: 0xFFFF.

        /// <param name="data">Массив байт, для которого вычисляется CRC.</param>
        /// <param name="length">Количество байт в массиве для вычисления (обычно длина пакета минус 2).</param>
        /// <returns>Вычисленное CRC16 значение.</returns>
        public static ushort Calculate(byte[] data, int length)
        {
            // Начальное значение CRC
            ushort crc = 0xFFFF;
            for (int pos = 0; pos < length; pos++)
            {
                // XOR текущего байта с младшим байтом CRC
                crc ^= data[pos];
                // Выполняем 8 итераций для каждого бита в байте
                for (int i = 0; i < 8; i++)
                {
                    // Если младший бит CRC равен 1, сдвигаем CRC и применяем полином
                    if ((crc & 1) != 0)
                    {
                        crc >>= 1; // Сдвиг вправо
                        crc ^= 0x8408; // XOR с отраженным полиномом
                    }
                    else
                    {
                        // Иначе просто сдвигаем вправо
                        crc >>= 1;
                    }
                }
            }
            return crc;
        }
    }
}