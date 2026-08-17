#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

class WiFiProvisioning {
private:
    static const uint32_t CONNECT_TIMEOUT_MS = 5000;

    Preferences _prefs;
    WebServer* _server = nullptr;

    bool _isConnected = false;
    bool _isAPMode = false;
    bool _provisioningDone = false;  // ← ДОБАВЛЕНО

    static const char* AP_SSID;
    static const char* AP_PASS;
    static const char* _htmlPage;

    void startAP();
    void handleRoot();
    void handleSave();
    void handleScan();

public:
    void begin();
    void handle();

    bool isConnected() const { return _isConnected; }
    bool isAPMode() const { return _isAPMode; }

    IPAddress getLocalIP() const;
    void printStatus() const;
};