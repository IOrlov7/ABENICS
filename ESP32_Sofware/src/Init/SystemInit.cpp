// src/Init/SystemInit.cpp
#include "Init/SystemInit.h"
#include "HAL/I2CBus.h"
#include "Sensors/SensorManager.h"
#include "Motors/TD7120MG/ServoController.h"
#include "Motors/Nema23/StepperController.h"
#include "Communication/WiFiProvisioning.h"
#include "Communication/NetworkManager.h"
#include "Control/CascadeControl.h"  // ★ НОВОЕ

// ============================================================
//  Глобальные объекты
// ============================================================
static ProjectConfig g_config;
ServoController g_servoCtrl;
StepperController g_stepperCtrl;
static WiFiProvisioning g_wifi;
NetworkManager g_network;
static bool g_systemReady = false;

volatile CommandId g_currentCommand = CommandId::CMD_IDLE;
volatile uint8_t g_statusFlags = 0;

extern void controlTask(void *parameter);

static void wifiTask(void *parameter)
{
    for (;;)
    {
        g_wifi.handle();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ============================================================
//  System_Init
// ============================================================
bool System_Init()
{
    Serial.println("=== ABENICS Controller Starting ===");
    Serial.println("[SysInit] === BEGIN ===");

    g_config = GetProjectConfig();
    Serial.printf("[SysInit] Config loaded. I2C: %d Hz, Telemetry: %d ms, Control: %d ms\n",
                  g_config.i2c.frequencyHz,
                  g_config.network.telemetryPeriodMs,
                  g_config.control.loopPeriodMs);

    auto &i2c = I2CBus::instance();
    if (!i2c.begin(g_config.i2c.sda, g_config.i2c.scl, g_config.i2c.frequencyHz))
    {
        Serial.println("[SysInit] I2C init FAILED");
        return false;
    }
    Serial.println("[SysInit] I2C init OK");
    i2c.scan();
    delay(100);

    if (!SensorManager::instance().begin(g_config.imu.mpu6500Address, Wire))
    {
        Serial.println("[SysInit] SensorManager init FAILED");
    }
    else
    {
        Serial.println("[SysInit] SensorManager init OK");
        Serial.println("[SysInit] 🔧 Starting IMU calibration — DO NOT MOVE the device!");
        SensorManager::instance().calibrate();
        Serial.println("[SysInit] ✅ IMU calibration completed");
    }

    if (g_servoCtrl.begin(
            g_config.servo.pca9685Address,
            g_config.servo.frequencyHz,
            g_config.servo.minUs,
            g_config.servo.maxUs,
            g_config.servo.count))
    {
        Serial.println("[SysInit] ServoController OK");
    }
    else
    {
        Serial.println("[SysInit] ServoController FAILED");
    }

    if (g_stepperCtrl.begin(g_config.stepper))
    {
        Serial.println("[SysInit] StepperController OK");
    }
    else
    {
        Serial.println("[SysInit] StepperController FAILED");
    }
    g_stepperCtrl.disableAll();

    // ★ НОВОЕ: Инициализация каскадного регулятора
    CascadeControl::GetInstance().Init();
    Serial.println("[SysInit] CascadeControl initialized");

    g_wifi.begin();
    g_wifi.printStatus();

    g_network.begin(g_config.network.telemetryPort, g_config.network.commandPort);
    Serial.println("[SysInit] NetworkManager OK");

    g_systemReady = true;
    Serial.println("[SysInit] === DONE ===");
    return g_systemReady;
}

// ============================================================
//  System_StartTasks
// ============================================================
void System_StartTasks()
{
    if (!g_systemReady)
        return;

    auto &cfg = g_config.control;

    // Задача управления (Core 1, приоритет 3)
    xTaskCreatePinnedToCore(
        controlTask, "ControlTask", 4096, nullptr,
        cfg.controlTaskPriority, nullptr, cfg.controlTaskCore);

    if (SensorManager::instance().isInitialized())
    {
        SensorManager::instance().startTask(cfg.sensorTaskCore, cfg.sensorTaskPriority);
    }
    else
    {
        Serial.println("[SysInit] Skipping SensorTask: SensorManager not ready.");
    }

    xTaskCreatePinnedToCore(
        wifiTask, "WifiTask", 4096, nullptr,
        cfg.wifiTaskPriority, nullptr, cfg.wifiTaskCore);

    g_network.startTask(cfg.networkTaskCore, cfg.networkTaskPriority);

    Serial.println("[SysInit] All tasks started");
    Serial.println("=== Setup complete. Joystick control ACTIVE. ===");
}

const ProjectConfig &System_GetConfig() { return g_config; }

uint32_t System_GetTelemetryPeriodMs()
{
    return g_config.network.telemetryPeriodMs;
}

uint32_t System_GetControlPeriodMs()
{
    return g_config.control.loopPeriodMs;
}

bool System_WiFiConnected()
{
    return g_wifi.isConnected();
}

namespace sys
{
    bool initAll() { return System_Init(); }
    void startTasks() { return System_StartTasks(); }
}