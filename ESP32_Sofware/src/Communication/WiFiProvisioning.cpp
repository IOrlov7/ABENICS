#include "Communication/WiFiProvisioning.h"

const char* WiFiProvisioning::AP_SSID = "ABENICS-Setup";
const char* WiFiProvisioning::AP_PASS = "12345678";

const char* WiFiProvisioning::_htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ABENICS WiFi Setup</title>
    <style>
        body{font-family:Arial;max-width:420px;margin:40px auto;padding:20px;background:#111;color:#eee}
        h1{color:#0f8;font-size:22px}
        input{width:100%;padding:12px;margin:6px 0 14px;box-sizing:border-box;background:#222;color:#eee;border:1px solid #444;border-radius:4px;font-size:15px}
        button{width:100%;padding:14px;background:#0f8;color:#000;border:none;border-radius:4px;font-weight:bold;font-size:16px;cursor:pointer}
        .info{color:#888;font-size:12px;margin-top:16px}
        .status{padding:8px;border-radius:4px;margin-bottom:12px;font-size:13px}
        .ok{background:#143;color:#0f8}
        .warn{background:#431;color:#fa0}
    </style>
</head>
<body>
    <h1>🤖 ABENICS WiFi Setup</h1>
    <div class="status warn">Робот продолжает работать с джойстика даже без Wi-Fi.</div>
    <form method='POST' action='/save'>
        <label>SSID сети:</label>
        <input type='text' name='ssid' required placeholder="Имя вашей Wi-Fi сети">
        <label>Пароль:</label>
        <input type='password' name='pass' placeholder="Пароль (можно пустой)">
        <button type='submit'>Сохранить и перезагрузить</button>
    </form>
    <p class="info">После сохранения ESP32 перезагрузится и подключится к сети.<br>
    Если пропустить — робот работает автономно с джойстика.</p>
    <p class="info"><a href="/scan" style="color:#0f8">📡 Сканировать сети</a></p>
</body>
</html>
)rawliteral";

void WiFiProvisioning::begin() {
    _prefs.begin("wifi-creds", false);

    String savedSSID = _prefs.getString("ssid", "");
    String savedPass = _prefs.getString("pass", "");

    // Если есть сохранённые данные — пробуем подключиться
    if (savedSSID.length() > 0) {
        Serial.printf("[WiFi] Trying saved network: '%s'\n", savedSSID.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(savedSSID.c_str(), savedPass.c_str());

        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < CONNECT_TIMEOUT_MS) {
            delay(100);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            _isConnected = true;
            _isAPMode = false;
            _provisioningDone = true;
            Serial.printf("[WiFi] ✓ Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            _prefs.end();
            return; // Успех — выходим, НЕ блокируем
        }

        Serial.println("[WiFi] ✗ Failed to connect within timeout.");
    } else {
        Serial.println("[WiFi] No saved credentials.");
    }

    // Не подключились — поднимаем AP в фоне, но НЕ блокируем
    startAP();
    _prefs.end();
}

void WiFiProvisioning::startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);

    _isConnected = false;
    _isAPMode = true;
    _provisioningDone = false;

    Serial.printf("[WiFi] AP started: '%s' (pass: %s)\n", AP_SSID, AP_PASS);
    Serial.printf("[WiFi] Config URL: http://%s\n", WiFi.softAPIP().toString().c_str());
    Serial.println("[WiFi] Robot continues to work with joystick. WiFi is optional.");

    _server = new WebServer(80);
    _server->on("/", [this]() { handleRoot(); });
    _server->on("/save", HTTP_POST, [this]() { handleSave(); });
    _server->on("/scan", [this]() { handleScan(); });
    _server->begin();
}

void WiFiProvisioning::handle() {
    // Вызывается в цикле wifiTask
    if (_isAPMode && _server) {
        _server->handleClient();
    }

    // Периодическая проверка: вдруг Wi-Fi переподключился
    if (_isConnected && WiFi.status() != WL_CONNECTED) {
        _isConnected = false;
        Serial.println("[WiFi] Connection lost!");
    }
}

void WiFiProvisioning::handleRoot() {
    _server->send(200, "text/html", _htmlPage);
}

void WiFiProvisioning::handleSave() {
    if (!_server->hasArg("ssid")) {
        _server->send(400, "text/plain", "Missing SSID");
        return;
    }

    String ssid = _server->arg("ssid");
    String pass = _server->arg("pass");

    _prefs.begin("wifi-creds", false);
    _prefs.putString("ssid", ssid);
    _prefs.putString("pass", pass);
    _prefs.end();

    _server->send(200, "text/html",
        "<html><body style='background:#111;color:#0f8;font-family:Arial;text-align:center;padding:50px'>"
        "<h1>✓ Сохранено!</h1><p>ESP32 перезагрузится через 2 секунды...</p>"
        "<script>setTimeout(()=>{location='/';},3000);</script></body></html>");

    delay(2000);
    ESP.restart();
}

void WiFiProvisioning::handleScan() {
    String html = "<html><body style='background:#111;color:#eee;font-family:Arial;padding:20px'>";
    html += "<h2>📡 Найденные сети:</h2><ul>";

    int n = WiFi.scanNetworks();
    if (n == 0) {
        html += "<li>Сети не найдены</li>";
    } else {
        for (int i = 0; i < n; i++) {
            html += "<li>" + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)";
            html += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " [OPEN]" : " [SECURED]";
            html += "</li>";
        }
    }

    html += "</ul><a href='/' style='color:#0f8'>← Назад</a></body></html>";
    _server->send(200, "text/html", html);
}

IPAddress WiFiProvisioning::getLocalIP() const {
    if (_isConnected) return WiFi.localIP();
    if (_isAPMode) return WiFi.softAPIP();
    return IPAddress(0, 0, 0, 0);
}

void WiFiProvisioning::printStatus() const {
    Serial.println("\n--- WiFi Status ---");
    if (_isConnected) {
        Serial.println("Mode:    STA (connected)");
        Serial.println("SSID:    " + WiFi.SSID());
        Serial.println("IP:      " + WiFi.localIP().toString());
        Serial.println("RSSI:    " + String(WiFi.RSSI()) + " dBm");
    } else if (_isAPMode) {
        Serial.println("Mode:    AP (provisioning)");
        Serial.println("AP SSID: " + String(AP_SSID));
        Serial.println("AP IP:   " + WiFi.softAPIP().toString());
    } else {
        Serial.println("Mode:    IDLE");
    }
    Serial.println("-------------------\n");
}