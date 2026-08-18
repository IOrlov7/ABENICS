#ifndef WIFI_PROVISIONING_H
#define WIFI_PROVISIONING_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <WebServer.h>

class WiFiProvisioning {
public:
    WiFiProvisioning() = default;
    ~WiFiProvisioning();

    void begin();
    void handle();

    bool isConnected() const { return _isConnected; }
    bool isAPMode() const { return _isAPMode; }
    IPAddress getLocalIP() const;
    void printStatus() const;

private:
    void startAP();
    void handleRoot();
    void handleSave();
    void handleScan();

    static void wifiEventCallback(WiFiEvent_t event, WiFiEventInfo_t info);
    void onWiFiConnected();
    void onWiFiDisconnected();
    void onWiFiGotIP();

    void checkButtonInLoop();
    void clearCredentials();
    void handleSerialCommands();
    void startSTA();

    bool        _isConnected      = false;
    bool        _isAPMode         = false;
    bool        _provisioningDone = false;
    uint32_t    _connectStartTime = 0;

    Preferences _prefs;
    WebServer*  _server = nullptr;

    // ★ ID зарегистрированного WiFi event (для корректного удаления)
    wifi_event_id_t _wifiEventId = 0;

    static const char* AP_SSID;
    static const char* AP_PASS;
    static const char* _htmlPage;

    static constexpr uint32_t CONNECT_TIMEOUT_MS = 10000;
    static constexpr uint8_t RESET_BUTTON_PIN = 25;
    
    static WiFiProvisioning* _instance;
};

#endif // WIFI_PROVISIONING_H