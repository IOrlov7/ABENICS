// serialport.cpp

#include "Communication/SerialPort.h"
#include <Arduino.h>
#include <esp_crc.h>
#include <cstring>

// Глобальный экземпляр SerialPort
SerialPort g_serial;

// ============================================================
// Конструктор
// ============================================================
SerialPort::SerialPort()
    : _ringHead(0), _ringTail(0), _binLen(0), _binHeaderFound(false), _readyBinLen(0), _binaryCommandReady(false), _textLen(0), _textCommandReady(false), _enabled(false), _baudrate(115200), _suppressDebug(false) // ★ НОВОЕ
{
}

// ============================================================
// Инициализация COM-порта
// ============================================================
void SerialPort::begin(uint32_t baudrate)
{
    _baudrate = baudrate;
    Serial.begin(baudrate);
    _enabled = true;
    _suppressDebug = true; // ★ ПОДАВЛЯЕМ отладку при активном COM
    // ★ ЗАМЕНЕНО: теперь используем макрос SERIAL_DEBUG
    SERIAL_DEBUG("[SERIAL] COM-port initialized at %u baud\n", baudrate);
}

bool SerialPort::isDebugSuppressed() const {
    return _suppressDebug;
}
// ============================================================
// Отправка бинарного пакета телеметрии
// ============================================================
void SerialPort::sendTelemetry(const TelemetryPacket &pkt)
{
    if (!_enabled)
        return;

    // Отладка (раз в 2 сек)
    static uint32_t lastDbg = 0;
    if (millis() - lastDbg > 2000)
    {
        // ★ ИЗМЕНЕНО: %u для uint32_t (было %hu для uint16_t)
        SERIAL_DEBUG("[SERIAL] TX pkt #%u\n", pkt.packet_id);
        lastDbg = millis();
    }

    Serial.write((const uint8_t *)&pkt, sizeof(TelemetryPacket));
}

// ============================================================
// Обработка входящих данных (вызывается в цикле)
// ============================================================
void SerialPort::processIncoming()
{
    if (!_enabled)
        return;

    // Читаем доступные байты из Serial
    readAvailable();

    // Разбираем поток на кадры
    parseStream();
}

// ============================================================
// Чтение доступных байт из Serial в кольцевой буфер
// ============================================================
void SerialPort::readAvailable()
{
    // Читаем все доступные байты из Serial
    while (Serial.available() > 0)
    {
        uint8_t byte = Serial.read();
        size_t nextHead = (_ringHead + 1) % RING_BUF_SIZE;

        // Проверка на переполнение кольцевого буфера
        if (nextHead == _ringTail)
        {
            // Буфер полон, пропускаем байт
            continue;
        }

        _ringBuf[_ringHead] = byte;
        _ringHead = nextHead;
    }
}

// ============================================================
// Разбор кольцевого буфера на бинарные и текстовые кадры
// ============================================================
void SerialPort::parseStream()
{
    // Обрабатываем все байты из кольцевого буфера
    while (_ringTail != _ringHead)
    {
        uint8_t byte = _ringBuf[_ringTail];
        _ringTail = (_ringTail + 1) % RING_BUF_SIZE;

        // Проверка: это начало бинарного пакета (0xAA)?
        if (!_binHeaderFound && _binLen == 0 && byte == TELEMETRY_HEADER_BYTE_0)
        {
            // Возможный заголовок
            _binBuf[0] = byte;
            _binLen = 1;
            continue;
        }

        // Проверка: второй байт заголовка (0x55)?
        if (!_binHeaderFound && _binLen == 1 && byte == TELEMETRY_HEADER_BYTE_1)
        {
            // Заголовок подтверждён
            _binBuf[1] = byte;
            _binLen = 2;
            _binHeaderFound = true;
            continue;
        }

        // Если нашли заголовок — накапливаем бинарный пакет
        if (_binHeaderFound)
        {
            if (_binLen < sizeof(_binBuf))
            {
                _binBuf[_binLen++] = byte;
            }

            // Проверка: пакет собран? (минимум 5 байт: header(2) + cmd(1) + crc(2))
            if (_binLen >= 5)
            {
                // Проверяем CRC
                uint16_t receivedCRC = (uint16_t)(_binBuf[_binLen - 2] << 8) | _binBuf[_binLen - 1];
                uint16_t calcCRC = calcCRC16(_binBuf, _binLen - 2);

                if (receivedCRC == calcCRC)
                {
                    // Пакет валиден — сохраняем для receiveBinaryCommand
                    memcpy(_readyBinBuf, _binBuf, _binLen);
                    _readyBinLen = _binLen;
                    _binaryCommandReady = true;

                    // Сбрасываем состояние
                    _binHeaderFound = false;
                    _binLen = 0;
                }
                else
                {
                    // CRC не совпадает — это не бинарный пакет
                    // Сбрасываем и пробуем найти заголовок заново
                    _binHeaderFound = false;
                    _binLen = 0;
                }
            }
        }
        else
        {
            // Это текстовый байт (не начинается с 0xAA)
            if (byte == '\n' || byte == '\r')
            {
                if (_textLen > 0)
                {
                    // Команда готова — сохраняем для receiveTextCommand
                    _textBuf[_textLen] = '\0';
                    strncpy(_readyTextBuf, _textBuf, sizeof(_readyTextBuf) - 1);
                    _readyTextBuf[sizeof(_readyTextBuf) - 1] = '\0';
                    _textCommandReady = true;
                    _textLen = 0;
                }
            }
            else
            {
                // Накопление текстовой команды
                if (_textLen < sizeof(_textBuf) - 1)
                {
                    _textBuf[_textLen++] = (char)byte;
                }
            }
        }
    }
}

// ============================================================
// Приём бинарной команды (неблокирующий)
// ============================================================
bool SerialPort::receiveBinaryCommand(uint8_t *cmdBuf, size_t bufSize, size_t &outLen)
{
    // Проверяем, есть ли готовый бинарный пакет
    if (!_binaryCommandReady)
    {
        return false;
    }

    // Копируем готовый пакет в буфер вызывающего кода
    size_t copyLen = (_readyBinLen < bufSize) ? _readyBinLen : bufSize;
    memcpy(cmdBuf, _readyBinBuf, copyLen);
    outLen = copyLen;

    // Сбрасываем флаг готовности
    _binaryCommandReady = false;
    return true;
}

// ============================================================
// Приём текстовой команды (неблокирующий)
// ============================================================
bool SerialPort::receiveTextCommand(char *buf, size_t bufSize)
{
    // Проверяем, есть ли готовая текстовая команда
    if (!_textCommandReady)
    {
        return false;
    }

    // Копируем готовую команду в буфер вызывающего кода
    strncpy(buf, _readyTextBuf, bufSize - 1);
    buf[bufSize - 1] = '\0';

    // Сбрасываем флаг готовности
    _textCommandReady = false;
    return true;
}

// ============================================================
// Статус: включен ли COM-порт
// ============================================================
bool SerialPort::isEnabled() const
{
    return _enabled;
}

// ============================================================
// CRC16 (полином 0x8408, начальное 0xFFFF)
// ============================================================
uint16_t SerialPort::calcCRC16(const uint8_t *data, size_t len) const
{
    return esp_crc16_le(UINT16_MAX, data, len);
}
