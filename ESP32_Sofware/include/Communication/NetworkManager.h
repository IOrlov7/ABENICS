#pragma once

#include <WiFi.h>
#include <WiFiUdp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Communication/TelemetryPacket.h"

// ★ МАКРОСЫ UDP_TELEMETRY_PORT / UDP_COMMAND_PORT УДАЛЕНЫ
// Порты теперь передаются из ProjectConfig через begin()

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    // Инициализация UDP-сокетов (порты передаются из конфига)
    void begin(uint16_t telemetryPort, uint16_t commandPort);

    // Запуск задачи networkTask
    bool startTask(uint8_t coreId = 0, uint8_t priority = 2);

    // Приём команды (неблокирующий)
    bool receiveCommand(void* cmdBuf, size_t bufSize);

    // Статус
    bool isConnected() const;

private:
    // Задача FreeRTOS
    TaskHandle_t _taskHandle;
    static void networkTaskEntry(void* param);
    void networkTaskLoop();

    // Сборка и отправка телеметрии
    void sendTelemetry();

    // CRC16
    uint16_t calcCRC16(const uint8_t* data, size_t len) const;
    void finalizePacket(TelemetryPacket& pkt);

    WiFiUDP _udpTelemetry;
    WiFiUDP _udpCommand;

    uint16_t _telemetryPort;   // ★ Сохраняем порт
    uint16_t _commandPort;     // ★ Сохраняем порт

    uint16_t _packetId;
    IPAddress _clientIP;
    bool _clientIPSet;
};

extern NetworkManager g_network;