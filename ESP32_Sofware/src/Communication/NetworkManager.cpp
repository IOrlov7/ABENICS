// networkmanager.cpp
#include "Communication/NetworkManager.h"
#include "Communication/SerialPort.h"
#include "Sensors/SensorManager.h"
#include "Init/SystemInit.h"
#include "Control/CascadeControl.h"  // ★ НОВОЕ: для PID-тюнинга
#include <esp_crc.h>
#include <cstring>

// ============================================================
//  Конструктор / деструктор
// ============================================================
NetworkManager::NetworkManager()
    : _taskHandle(nullptr), _packetId(0), _clientIPSet(false), _telemetryPort(8888)
    , _commandPort(8889)
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
    SERIAL_DEBUG("[NET] UDP sockets: %d (tx) / %d (rx)\n", _telemetryPort, _commandPort);
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
        SERIAL_DEBUG("[NET] Task creation FAILED\n");
        return false;
    }
    SERIAL_DEBUG("[NET] Task started\n");
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
    SERIAL_DEBUG("[NET] Loop started, period=%u ms\n", periodMs);

    for (;;)
    {
        g_serial.processIncoming();

        // 1. Приём команд от ПК (UDP)
        uint8_t cmdBuf[64];
        if (receiveCommand(cmdBuf, sizeof(cmdBuf)))
        {
            SERIAL_DEBUG("[NET] Command received: 0x%02X\n", cmdBuf[0]);
            // TODO: парсинг бинарных команд
        }

        // ★ НОВОЕ: Приём текстовых PID-команд по UDP
        uint8_t textBuf[256];
        int textLen = _udpCommand.parsePacket();
        if (textLen > 0)
        {
            textLen = _udpCommand.read(textBuf, sizeof(textBuf) - 1);
            if (textLen > 0)
            {
                textBuf[textLen] = '\0';
                String cmd = String((char*)textBuf);
                processPidCommand(cmd);  // ★ НОВОЕ
            }
        }

        // Приём бинарных команд по Serial
        uint8_t serialCmdBuf[64];
        size_t serialCmdLen;
        if (g_serial.receiveBinaryCommand(serialCmdBuf, sizeof(serialCmdBuf), serialCmdLen))
        {
            SERIAL_DEBUG("[NET] Serial binary command received: 0x%02X\n", serialCmdBuf[2]);
            // TODO: обработка бинарной команды
        }

        // Приём текстовых команд по Serial
        char textCmdBuf[128];
        if (g_serial.receiveTextCommand(textCmdBuf, sizeof(textCmdBuf)))
        {
            SERIAL_DEBUG("[NET] Serial text command: %s\n", textCmdBuf);
            // ★ НОВОЕ: обработка PID-команд по Serial тоже
            String serialCmd = String(textCmdBuf);
            processPidCommand(serialCmd);
        }

        // 2. Отправка телеметрии
        // Сер.: телеметрия всегда уходит по USB (COM), независимо от WiFi
        // UDP: только при подключённом WiFi
        sendTelemetry();

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

    // IMU
    pkt.imu.quat_w = l_mpuData.quatW;
    pkt.imu.quat_x = l_mpuData.quatX;
    pkt.imu.quat_y = l_mpuData.quatY;
    pkt.imu.quat_z = l_mpuData.quatZ;
    pkt.imu.roll = l_mpuData.roll;
    pkt.imu.pitch = l_mpuData.pitch;
    pkt.imu.yaw = l_mpuData.yaw;
    pkt.imu.accel_x = l_mpuData.accelX;
    pkt.imu.accel_y = l_mpuData.accelY;
    pkt.imu.accel_z = l_mpuData.accelZ;
    pkt.imu.gyro_x = l_mpuData.gyroX;
    pkt.imu.gyro_y = l_mpuData.gyroY;
    pkt.imu.gyro_z = l_mpuData.gyroZ;
    pkt.imu.mag_x = l_mpuData.magX;
    pkt.imu.mag_y = l_mpuData.magY;
    pkt.imu.mag_z = l_mpuData.magZ;

    // Моторы (заглушки, пока нет энкодеров)
    pkt.stepper_x_angle = 0.0f;  // TODO: SensorManager::getEncoderAngleX()
    pkt.stepper_y_angle = 0.0f;  // TODO: SensorManager::getEncoderAngleY()
    for (int i = 0; i < 8; i++)
        pkt.servo_angles[i] = 0;

    // ★ НОВОЕ: Заполнение полей для PID-тюнинга
    auto& ctrl = CascadeControl::GetInstance();
    pkt.target_angle_x = ctrl.GetAxisX().targetEncoderAngle;
    pkt.target_angle_y = ctrl.GetAxisY().targetEncoderAngle;
    pkt.target_speed_x = ctrl.GetAxisX().motorOutput;
    pkt.target_speed_y = ctrl.GetAxisY().motorOutput;

    // Система
    pkt.system.current_cmd = (uint8_t)g_currentCommand;
    pkt.system.status_flags = g_statusFlags;
    pkt.system.wifi_rssi = (int8_t)WiFi.RSSI();

    // Заголовок + CRC
    finalizePacket(pkt);

    // Отладка
    static uint32_t lastDbg = 0;
    if (millis() - lastDbg > 5000)
    {
        SERIAL_DEBUG("[NET] TX #%u → %s | accel=(%.2f, %.2f, %.2f) | size=%d\n",
                     pkt.packet_id,
                     (_clientIPSet ? _clientIP.toString().c_str() : "BROADCAST"),
                     pkt.imu.accel_x, pkt.imu.accel_y, pkt.imu.accel_z,
                     (int)sizeof(TelemetryPacket));
        lastDbg = millis();
    }

    // Сер. (USB COM): телеметрия отправляется ВСЕГДА, независимо от WiFi
    g_serial.sendTelemetry(pkt);

    // UDP: только при подключённом WiFi
    if (isConnected())
    {
        IPAddress destIP = _clientIPSet ? _clientIP : IPAddress(255, 255, 255, 255);
        _udpTelemetry.beginPacket(destIP, _telemetryPort);
        _udpTelemetry.write((const uint8_t *)&pkt, sizeof(TelemetryPacket));
        _udpTelemetry.endPacket();
    }
}

// ============================================================
//  ★ НОВОЕ: Парсер PID-команд
// ============================================================
void NetworkManager::processPidCommand(const String& cmd)
{
    auto& ctrl = CascadeControl::GetInstance();
    PIDCoeffs c;

    // Формат: "PID:OX:1.5,0.1,0.05,0.0" (Outer X)
    //         "PID:IX:2.0,0.5,0.0,0.3"  (Inner X)
    //         "PID:OY:..."               (Outer Y)
    //         "PID:IY:..."               (Inner Y)
    //         "PID:TGT:5.0,-3.0"         (Target pitch, roll)

    if (cmd.startsWith("PID:OX:"))
    {
        if (parseCoeffs(cmd.substring(7), c))
        {
            ctrl.SetOuterCoeffsX(c);
            SERIAL_DEBUG("[NET] PID Outer X updated: Kp=%.2f, Ki=%.2f, Kd=%.2f, Kff=%.2f\n",
                         c.Kp, c.Ki, c.Kd, c.Kff);
        }
    }
    else if (cmd.startsWith("PID:IX:"))
    {
        if (parseCoeffs(cmd.substring(7), c))
        {
            ctrl.SetInnerCoeffsX(c);
            SERIAL_DEBUG("[NET] PID Inner X updated: Kp=%.2f, Ki=%.2f, Kd=%.2f, Kff=%.2f\n",
                         c.Kp, c.Ki, c.Kd, c.Kff);
        }
    }
    else if (cmd.startsWith("PID:OY:"))
    {
        if (parseCoeffs(cmd.substring(7), c))
        {
            ctrl.SetOuterCoeffsY(c);
            SERIAL_DEBUG("[NET] PID Outer Y updated: Kp=%.2f, Ki=%.2f, Kd=%.2f, Kff=%.2f\n",
                         c.Kp, c.Ki, c.Kd, c.Kff);
        }
    }
    else if (cmd.startsWith("PID:IY:"))
    {
        if (parseCoeffs(cmd.substring(7), c))
        {
            ctrl.SetInnerCoeffsY(c);
            SERIAL_DEBUG("[NET] PID Inner Y updated: Kp=%.2f, Ki=%.2f, Kd=%.2f, Kff=%.2f\n",
                         c.Kp, c.Ki, c.Kd, c.Kff);
        }
    }
    else if (cmd.startsWith("PID:TGT:"))
    {
        int comma = cmd.indexOf(',', 8);
        if (comma > 0)
        {
            float pitch = cmd.substring(8, comma).toFloat();
            float roll = cmd.substring(comma + 1).toFloat();
            ctrl.SetTarget(pitch, roll);
            SERIAL_DEBUG("[NET] PID Target updated: pitch=%.2f, roll=%.2f\n", pitch, roll);
        }
    }
}

bool NetworkManager::parseCoeffs(const String& s, PIDCoeffs& c)
{
    // Формат: "1.5,0.1,0.05,0.0"
    int i1 = s.indexOf(',');
    if (i1 < 0) return false;
    int i2 = s.indexOf(',', i1 + 1);
    if (i2 < 0) return false;
    int i3 = s.indexOf(',', i2 + 1);
    if (i3 < 0) return false;

    c.Kp = s.substring(0, i1).toFloat();
    c.Ki = s.substring(i1 + 1, i2).toFloat();
    c.Kd = s.substring(i2 + 1, i3).toFloat();
    c.Kff = s.substring(i3 + 1).toFloat();
    return true;
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

    uint16_t receivedCRC = (uint16_t)(rxBuffer[readLen - 1] << 8) | rxBuffer[readLen - 2];
    uint16_t calcCRC = calcCRC16(rxBuffer, readLen - 2);

    if (receivedCRC != calcCRC)
    {
        Serial.printf("[NET] ⚠ CMD CRC mismatch: got 0x%04X, calc 0x%04X, len=%d\n",
                      receivedCRC, calcCRC, (int)readLen);
        return false;
    }

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
    pkt.crc16 = calcCRC16((const uint8_t *)&pkt, sizeof(TelemetryPacket) - 2);
}