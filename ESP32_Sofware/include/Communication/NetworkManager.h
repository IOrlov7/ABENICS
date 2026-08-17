#pragma once
#include <WiFi.h>
#include <WiFiUdp.h>
#include "TelemetryPacket.h"

// Структура команд от ПК
#pragma pack(push, 1)
struct CommandPacket {
    uint8_t header[4] = {'C', 'M', 'D', '!'};
    uint8_t commandType;    // 1=ESTOP, 2=SET_MODE, 3=CALIBRATE
    float payload[4];
    uint16_t checksum;
};
#pragma pack(pop)

class NetworkManager {
private:
    WiFiUDP _udpTelemetry;
    WiFiUDP _udpCommands;

    uint16_t _telemetryPort = 8888;
    uint16_t _commandPort   = 8889;

    IPAddress _targetIP;
    uint32_t _packetId = 0;
    unsigned long _successCount = 0;
    unsigned long _errorCount = 0;

public:
    NetworkManager(const IPAddress& defaultIP = IPAddress(255, 255, 255, 255))
        : _targetIP(defaultIP) {}

    // ← ИСПРАВЛЕНО: begin() без аргументов.
    // Wi-Fi уже подключен через WiFiProvisioning.
    // Здесь только открываем UDP-сокеты.
    void begin() {
        _udpTelemetry.begin(_telemetryPort);
        _udpCommands.begin(_commandPort);
        Serial.printf("[UDP] Telemetry port: %d, Command port: %d\n",
                      _telemetryPort, _commandPort);
    }

    void setTargetIP(const IPAddress& ip) {
        _targetIP = ip;
        Serial.println("[UDP] Target IP set to: " + _targetIP.toString());
    }

    bool sendTelemetry(TelemetryPacket& packet) {
        if (WiFi.status() != WL_CONNECTED) return false;

        packet.packetId = ++_packetId;
        packet.timestamp = millis();
        packet.wifi_rssi = WiFi.RSSI();

        _udpTelemetry.beginPacket(_targetIP, _telemetryPort);
        _udpTelemetry.write((uint8_t*)&packet, sizeof(TelemetryPacket));
        int result = _udpTelemetry.endPacket();

        if (result == 1) {
            _successCount++;
            return true;
        } else {
            _errorCount++;
            return false;
        }
    }

    bool receiveCommand(CommandPacket& cmd) {
        int packetSize = _udpCommands.parsePacket();
        if (packetSize == sizeof(CommandPacket)) {
            _udpCommands.read((uint8_t*)&cmd, sizeof(CommandPacket));
            // Проверка заголовка
            if (cmd.header[0] == 'C' && cmd.header[1] == 'M' &&
                cmd.header[2] == 'D' && cmd.header[3] == '!') {
                return true;
            }
        }
        return false;
    }

    void printStatus() const {
        Serial.println("\n--- UDP Status ---");
        Serial.printf("WiFi Connected: %s\n",
                      WiFi.status() == WL_CONNECTED ? "YES" : "NO");
        Serial.println("Local IP:       " + WiFi.localIP().toString());
        Serial.println("Target IP:      " + _targetIP.toString());
        Serial.printf("Packets OK:     %lu\n", _successCount);
        Serial.printf("Packets Failed: %lu\n", _errorCount);
        Serial.println("------------------\n");
    }
};