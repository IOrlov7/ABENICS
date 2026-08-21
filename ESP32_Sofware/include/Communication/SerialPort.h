// serialport.h

#pragma once
#include <stdint.h>
#include <stddef.h>
#include "Communication/TelemetryPacket.h"

// ============================================================
// SerialPort — модуль для работы с COM-портом (USB CDC)
// ============================================================
// Отвечает за:
// - Отправку бинарной телеметрии (103 байта) через Serial
// - Приём бинарных команд от клиента C# (формат [0xAA, 0x55, CMD, payload, CRC])
// - Приём текстовых команд (WIFI_SET и т.д.) из Serial Monitor
// - Мультиплексирование потоков: бинарные пакеты начинаются с 0xAA 0x55,
//   текстовые команды — с любых других символов
// ============================================================

class SerialPort {
public:
    SerialPort();
    
    // Инициализация COM-порта
    void begin(uint32_t baudrate = 115200);
    
    // Отправка бинарного пакета телеметрии
    void sendTelemetry(const TelemetryPacket& pkt);
    
    // Обработка входящих данных (вызывается в цикле networkTask)
    void processIncoming();
    
    // Приём бинарной команды (неблокирующий)
    bool receiveBinaryCommand(uint8_t* cmdBuf, size_t bufSize, size_t& outLen);
    
    // Приём текстовой команды (неблокирующий)
    bool receiveTextCommand(char* buf, size_t bufSize);
    
    // Статус
    bool isEnabled() const;
    
    // ★ НОВОЕ: флаг подавления отладки через Serial
    bool isDebugSuppressed() const;
    
private:
    // Кольцевой буфер для накопления входящих байт из Serial
    static const size_t RING_BUF_SIZE = 512;
    uint8_t _ringBuf[RING_BUF_SIZE];
    volatile size_t _ringHead;
    volatile size_t _ringTail;
    
    // Буфер для сборки бинарного пакета
    uint8_t _binBuf[256];
    size_t _binLen;
    bool _binHeaderFound;
    
    // Готовый бинарный пакет (после проверки CRC)
    uint8_t _readyBinBuf[256];
    size_t _readyBinLen;
    bool _binaryCommandReady;
    
    // Буфер для текстовой команды
    char _textBuf[128];
    size_t _textLen;
    
    // Готовая текстовая команда
    char _readyTextBuf[128];
    bool _textCommandReady;
    
    bool _enabled;
    uint32_t _baudrate;
    
    // ★ НОВОЕ: флаг подавления отладки
    bool _suppressDebug;
    
    // Чтение доступных байт из Serial в кольцевой буфер
    void readAvailable();
    
    // Разбор кольцевого буфера на бинарные и текстовые кадры
    void parseStream();
    
    // CRC16 (копия из NetworkManager для независимости)
    uint16_t calcCRC16(const uint8_t* data, size_t len) const;
};

extern SerialPort g_serial;

// ★ МАКРОС для безопасной отладки
// Если COM-канал активен — отладка НЕ пишется в Serial
#define SERIAL_DEBUG(fmt, ...) \
    do { \
        if (!g_serial.isDebugSuppressed()) { \
            Serial.printf(fmt, ##__VA_ARGS__); \
        } \
    } while(0)
