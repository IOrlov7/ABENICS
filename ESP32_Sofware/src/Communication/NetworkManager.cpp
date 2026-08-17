#include "Communication/NetworkManager.h"
#include <esp_crc.h>
#include <cstring>

// ============================================================
//  Конструктор / деструктор
// ============================================================

NetworkManager::NetworkManager()
    : _packetId(0)
    , _clientIPSet(false)
{
    _mutex = xSemaphoreCreateMutex();
}

NetworkManager::~NetworkManager() {
    if (_mutex) vSemaphoreDelete(_mutex);
}

// ============================================================
//  Инициализация
// ============================================================

void NetworkManager::begin() {
    // Сокет для отправки телеметрии (порт 8888)
    _udpTelemetry.begin(UDP_TELEMETRY_PORT);

    // Сокет для приёма команд (порт 8889)
    _udpCommand.begin(UDP_COMMAND_PORT);

    Serial.println("[NET] UDP sockets opened: 8888 (tx) / 8889 (rx)");
}

// ============================================================
//  Отправка телеметрии
// ============================================================

bool NetworkManager::sendTelemetry(TelemetryPacket& pkt) {
    if (!WiFi.isConnected()) return false;

    // Заполнить заголовок, packet_id, timestamp, CRC
    finalizePacket(pkt);

    // Определить адрес получателя
    IPAddress destIP;
    if (_clientIPSet) {
        destIP = _clientIP;
    } else {
        // Broadcast если клиент ещё не известен
        destIP = IPAddress(255, 255, 255, 255);
    }

    _udpTelemetry.beginPacket(destIP, UDP_TELEMETRY_PORT);
    _udpTelemetry.write((const uint8_t*)&pkt, sizeof(TelemetryPacket));
    _udpTelemetry.endPacket();

    return true;
}

// ============================================================
//  Приём команды
// ============================================================

bool NetworkManager::receiveCommand(void* cmdBuf, size_t bufSize) {
    int packetSize = _udpCommand.parsePacket();
    if (packetSize <= 0) return false;

    // Читаем пакет
    uint8_t rxBuffer[256];
    size_t readLen = _udpCommand.read(rxBuffer, sizeof(rxBuffer));

    // Запомнить IP клиента (для последующей отправки телеметрии)
    _clientIP = _udpCommand.remoteIP();
    _clientIPSet = true;

    // Минимальная проверка: заголовок + CRC
    if (readLen < 4) return false;

    // Проверить заголовок
    if (rxBuffer[0] != TELEMETRY_HEADER_BYTE_0 ||
        rxBuffer[1] != TELEMETRY_HEADER_BYTE_1) {
        return false;
    }

    // Проверить CRC (последние 2 байта — контрольная сумма)
    uint16_t receivedCRC = (uint16_t)(rxBuffer[readLen - 2] << 8) |
                           rxBuffer[readLen - 1];
    uint16_t calcCRC = calcCRC16(rxBuffer, readLen - 2);

    if (receivedCRC != calcCRC) {
        Serial.println("[NET] Command CRC mismatch!");
        return false;
    }

    // Скопировать полезную нагрузку в буфер вызывающего
    size_t payloadSize = readLen - 2; // минус CRC
    if (payloadSize > bufSize) payloadSize = bufSize;
    memcpy(cmdBuf, rxBuffer, payloadSize);

    return true;
}

// ============================================================
//  Статус
// ============================================================

bool NetworkManager::isConnected() const {
    return WiFi.isConnected();
}

// ============================================================
//  Внутренние методы
// ============================================================

uint16_t NetworkManager::calcCRC16(const uint8_t* data, size_t len) const {
    // Используем встроенную ESP-IDF функцию
    // Полином 0x8408 (reflected CCITT), начальное значение 0xFFFF
    return esp_crc16_le(UINT16_MAX, data, len);
}

void NetworkManager::finalizePacket(TelemetryPacket& pkt) {
    // Заголовок
    pkt.header[0] = TELEMETRY_HEADER_BYTE_0;
    pkt.header[1] = TELEMETRY_HEADER_BYTE_1;

    // ID пакета (инкремент, wrap-around)
    pkt.packet_id = _packetId++;

    // Timestamp
    pkt.timestamp_ms = millis();

    // CRC16 считается для всего пакета КРОМЕ поля crc16
    size_t crcLen = sizeof(TelemetryPacket) - sizeof(uint16_t);
    pkt.crc16 = calcCRC16((const uint8_t*)&pkt, crcLen);
}