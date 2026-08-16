#pragma once
#include <WiFi.h>
#include <WiFiUdp.h>
#include "DataPacket.h"

class UdpSender {
private:
    WiFiUDP _udp;
    uint16_t _port = 8888;
    IPAddress _targetIP;
    unsigned long _successCount = 0;
    unsigned long _errorCount = 0;

public:
    UdpSender(const IPAddress& defaultIP = IPAddress(255, 255, 255, 255)) 
        : _targetIP(defaultIP) {}

    void begin() {
        _udp.begin(_port);
        Serial.println("[UDP] Listener started on port " + String(_port));
    }

    void setTargetIP(const IPAddress& ip) {
        _targetIP = ip;
        Serial.println("[UDP] Target IP set to: " + _targetIP.toString());
    }

    // Возвращает true, если отправка успешна
    bool send(const DataPacket& packet) {
        // ГЛАВНОЕ ИСПРАВЛЕНИЕ: Не пытаемся отправлять, если WiFi не подключен
        if (WiFi.status() != WL_CONNECTED) {
            return false; 
        }

        _udp.beginPacket(_targetIP, _port);
        _udp.print(packet.toString());
        int result = _udp.endPacket();

        if (result == 1) {
            _successCount++;
            return true;
        } else {
            _errorCount++;
            // Защита от спама в Serial: печатаем ошибку только каждую 100-ю попытку
            if (_errorCount % 100 == 1) {
                Serial.printf("[UDP WARN] Send failed (err=%d). Target: %s\n", 
                              result, _targetIP.toString().c_str());
            }
            return false;
        }
    }

    void printStatus() const {
        Serial.println("\n--- UDP Status ---");
        Serial.println("WiFi Connected: " + String(WiFi.status() == WL_CONNECTED ? "YES" : "NO"));
        Serial.println("Local IP:       " + WiFi.localIP().toString());
        Serial.println("Target IP:      " + _targetIP.toString());
        Serial.println("Packets Sent:   " + String(_successCount));
        Serial.println("Packets Failed: " + String(_errorCount));
        Serial.println("------------------\n");
    }
};