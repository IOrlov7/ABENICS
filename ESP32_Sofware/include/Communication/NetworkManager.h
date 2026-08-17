#pragma once

#include <WiFi.h>
#include <WiFiUdp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "TelemetryPacket.h"

// Порты (из архитектуры)
#define UDP_TELEMETRY_PORT  8888
#define UDP_COMMAND_PORT    8889

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    // Инициализация UDP-сокетов
    void begin();

    // Отправка телеметрии (вызывается из networkTask)
    // Возвращает true если пакет успешно отправлен
    bool sendTelemetry(TelemetryPacket& pkt);

    // Приём команды (вызывается из networkTask)
    // Возвращает true если пакет получен и CRC валидна
    bool receiveCommand(void* cmdBuf, size_t bufSize);

    // Статус
    bool isConnected() const;

private:
    // Расчёт CRC16 для пакета (используем встроенный esp_crc.h)
    uint16_t calcCRC16(const uint8_t* data, size_t len) const;

    // Заполнить заголовок и CRC в пакете
    void finalizePacket(TelemetryPacket& pkt);

    WiFiUDP _udpTelemetry;   // Порт 8888 (телеметрия, отправка)
    WiFiUDP _udpCommand;     // Порт 8889 (команды, приём)

    uint16_t _packetId;      // Инкрементируемый ID пакета

    // Мьютекс для защиты shared data (IMU, моторы)
    SemaphoreHandle_t _mutex;

    // IP-адрес клиента (куда отправляем телеметрию)
    // Определяется при получении первой команды или задаётся вручную
    IPAddress _clientIP;
    bool _clientIPSet;
};

// Глобальный экземпляр (объявлен в SystemInit.cpp)
extern NetworkManager g_network;