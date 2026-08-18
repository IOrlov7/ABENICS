#include "Communication/NetworkManager.h"
#include "Sensors/SensorManager.h"
#include "Init/SystemInit.h"
#include <esp_crc.h>
#include <cstring>

// ============================================================
//  Конструктор / деструктор
// ============================================================

NetworkManager::NetworkManager()
    : _taskHandle(nullptr), _packetId(0), _clientIPSet(false), _telemetryPort(8888) // Дефолт на случай если begin() не вызван
      ,
      _commandPort(8889)
{
}

NetworkManager::~NetworkManager()
{
    if (_taskHandle)
        vTaskDelete(_taskHandle);
}

// ============================================================
//  Инициализация (порты из конфига)
// ============================================================

void NetworkManager::begin(uint16_t telemetryPort, uint16_t commandPort)
{
    _telemetryPort = telemetryPort;
    _commandPort = commandPort;

    _udpTelemetry.begin(_telemetryPort);
    _udpCommand.begin(_commandPort);

    Serial.printf("[NET] UDP sockets: %d (tx) / %d (rx)\n", _telemetryPort, _commandPort);
}

// ============================================================
//  Запуск задачи
// ============================================================

bool NetworkManager::startTask(uint8_t coreId, uint8_t priority)
{
    BaseType_t ok = xTaskCreatePinnedToCore(
        networkTaskEntry,
        "NetworkTask",
        8192,
        this,
        priority,
        &_taskHandle,
        coreId);
    if (ok != pdPASS)
    {
        Serial.println("[NET] Task creation FAILED");
        return false;
    }
    Serial.println("[NET] Task started");
    return true;
}

void NetworkManager::networkTaskEntry(void *param)
{
    static_cast<NetworkManager *>(param)->networkTaskLoop();
}

// ============================================================
//  Основной цикл задачи
// ============================================================

void NetworkManager::networkTaskLoop()
{
    uint32_t periodMs = System_GetTelemetryPeriodMs();
    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        // 1. Приём команд от ПК
        uint8_t cmdBuf[64];
        if (receiveCommand(cmdBuf, sizeof(cmdBuf)))
        {
            Serial.printf("[NET] Command received: 0x%02X\n", cmdBuf[0]);
            // TODO: парсинг команд (Этап 3)
        }

        // 2. Отправка телеметрии
        if (isConnected())
        {
            sendTelemetry();
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(periodMs));
    }
}

// ============================================================
//  Сборка и отправка телеметрии
// ============================================================

void NetworkManager::sendTelemetry()
{
    TelemetryPacket pkt;
    memset(&pkt, 0, sizeof(pkt));

    // --- IMU (из SensorManager) ---
    auto &sensors = SensorManager::instance();
    pkt.imu.quat_w = sensors.getQuatW();
    pkt.imu.quat_x = sensors.getQuatX();
    pkt.imu.quat_y = sensors.getQuatY();
    pkt.imu.quat_z = sensors.getQuatZ();
    pkt.imu.roll = sensors.getRoll();
    pkt.imu.pitch = sensors.getPitch();
    pkt.imu.yaw = sensors.getYaw();
    pkt.imu.accel_x = sensors.getAccelX();
    pkt.imu.accel_y = sensors.getAccelY();
    pkt.imu.accel_z = sensors.getAccelZ();
    pkt.imu.gyro_x = sensors.getGyroX();
    pkt.imu.gyro_y = sensors.getGyroY();
    pkt.imu.gyro_z = sensors.getGyroZ();
    pkt.imu.mag_x = sensors.getMagX();
    pkt.imu.mag_y = sensors.getMagY();
    pkt.imu.mag_z = sensors.getMagZ();
    pkt.imu.temperature = sensors.getTemperature();

    // --- Шаговые моторы (заглушки, пока нет API) ---
    pkt.stepper_x.angle = 0.0f;
    pkt.stepper_x.speed = 0.0f;
    pkt.stepper_x.direction = 0;
    pkt.stepper_x.state = 0;

    pkt.stepper_y.angle = 0.0f;
    pkt.stepper_y.speed = 0.0f;
    pkt.stepper_y.direction = 0;
    pkt.stepper_y.state = 0;

    // --- Сервоприводы (заглушки) ---
    for (int i = 0; i < 8; i++)
    {
        pkt.servo_angles[i] = 0;
    }

    // --- Системное состояние ---
    pkt.system.current_cmd = (uint8_t)g_currentCommand;
    pkt.system.status_flags = g_statusFlags;
    pkt.system.wifi_rssi = (int8_t)WiFi.RSSI();
    pkt.system.uptime_ms = millis();

    // --- Заголовок + CRC + отправка ---
    finalizePacket(pkt);

    IPAddress destIP = _clientIPSet ? _clientIP : IPAddress(255, 255, 255, 255);
    _udpTelemetry.beginPacket(destIP, _telemetryPort); // ★ используем _telemetryPort
    _udpTelemetry.write((const uint8_t *)&pkt, sizeof(TelemetryPacket));
    _udpTelemetry.endPacket();
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 2000)
    {
        Serial.printf("[NET] TX pkt #%u → %s | accel=(%.2f, %.2f, %.2f)\n",
                      _packetId - 1,
                      destIP.toString().c_str(),
                      pkt.imu.accel_x, pkt.imu.accel_y, pkt.imu.accel_z);
        lastPrint = millis();
    }
}

// ============================================================
//  Приём команд
// ============================================================

bool NetworkManager::receiveCommand(void *cmdBuf, size_t bufSize)
{
    int packetSize = _udpCommand.parsePacket();
    if (packetSize <= 0)
        return false;

    uint8_t rxBuffer[256];
    size_t readLen = _udpCommand.read(rxBuffer, sizeof(rxBuffer));

    _clientIP = _udpCommand.remoteIP();
    _clientIPSet = true;

    if (readLen < 4)
        return false;
    if (rxBuffer[0] != TELEMETRY_HEADER_BYTE_0 || rxBuffer[1] != TELEMETRY_HEADER_BYTE_1)
        return false;

    uint16_t receivedCRC = (uint16_t)(rxBuffer[readLen - 2] << 8) | rxBuffer[readLen - 1];
    uint16_t calcCRC = calcCRC16(rxBuffer, readLen - 2);
    if (receivedCRC != calcCRC)
        return false;

    size_t payloadSize = readLen - 2;
    if (payloadSize > bufSize)
        payloadSize = bufSize;
    memcpy(cmdBuf, rxBuffer, payloadSize);
    return true;
}

// ============================================================
//  Вспомогательные методы
// ============================================================

bool NetworkManager::isConnected() const
{
    return WiFi.isConnected();
}

uint16_t NetworkManager::calcCRC16(const uint8_t *data, size_t len) const
{
    return esp_crc16_le(UINT16_MAX, data, len);
}

void NetworkManager::finalizePacket(TelemetryPacket &pkt)
{
    pkt.header[0] = TELEMETRY_HEADER_BYTE_0;
    pkt.header[1] = TELEMETRY_HEADER_BYTE_1;
    pkt.packet_id = _packetId++;
    pkt.timestamp_ms = millis();

    size_t crcLen = sizeof(TelemetryPacket) - sizeof(uint16_t);
    pkt.crc16 = calcCRC16((const uint8_t *)&pkt, crcLen);
}