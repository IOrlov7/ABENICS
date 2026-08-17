#include "Init/SystemInit.h"

#include "HAL/I2CBus.h"
#include "Sensors/SensorManager.h"
#include "Motors/TD7120MG/ServoController.h"
#include "Motors/Nema23/StepperController.h"
#include "Communication/WiFiProvisioning.h"
#include "Communication/NetworkManager.h"

// ============================================================
//  Глобальные объекты
// ============================================================

static ProjectConfig g_config;

ServoController    g_servoCtrl;
StepperController  g_stepperCtrl;
static WiFiProvisioning g_wifi;
NetworkManager     g_network;

static bool g_systemReady = false;

// --- Глобальные переменные состояния ---
volatile CommandId g_currentCommand = CommandId::CMD_IDLE;
volatile uint8_t   g_statusFlags    = 0;

// --- Внешняя задача управления (определена в main.cpp) ---
extern void controlTask(void* parameter);

// --- Задача Wi-Fi (тривиальная обёртка) ---
static void wifiTask(void* parameter) {
    for (;;) {
        g_wifi.handle();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ============================================================
//  System_Init — ТОЛЬКО инициализация, ничего больше
// ============================================================

bool System_Init() {
    Serial.println("[SysInit] === BEGIN ===");

    // 1. Загрузка конфигурации
    g_config = GetProjectConfig();
    Serial.printf("[SysInit] Config loaded. I2C: %d Hz, Telemetry: %d ms, Control: %d ms\n",
                  g_config.i2c.frequencyHz,
                  g_config.network.telemetryPeriodMs,
                  g_config.control.loopPeriodMs);
    Serial.printf("[SysInit] Tasks: Ctrl@Core%d, Sensor@Core%d, Net@Core%d, WiFi@Core%d\n",
                  g_config.control.controlTaskCore,
                  g_config.control.sensorTaskCore,
                  g_config.control.networkTaskCore,
                  g_config.control.wifiTaskCore);

    // 2. Инициализация шины I2C
    auto& i2c = I2CBus::instance();
    if (!i2c.begin(g_config.i2c.sda, g_config.i2c.scl, g_config.i2c.frequencyHz)) {
        Serial.println("[SysInit] I2C init FAILED");
        return false;
    }
    Serial.println("[SysInit] I2C init OK");
    i2c.scan();

    // 3. Инициализация датчиков
    if (!SensorManager::instance().begin(g_config)) {
        Serial.println("[SysInit] SensorManager init FAILED");
    }

    // 4. Инициализация сервоприводов
    if (g_servoCtrl.begin(
        g_config.servo.pca9685Address,
        g_config.servo.frequencyHz,
        g_config.servo.minUs,
        g_config.servo.maxUs,
        g_config.servo.count)) {
        Serial.println("[SysInit] ServoController OK");
    } else {
        Serial.println("[SysInit] ServoController FAILED");
    }

    // 5. Инициализация шаговых моторов
    if (g_stepperCtrl.begin(g_config.stepper)) {
        Serial.println("[SysInit] StepperController OK");
    } else {
        Serial.println("[SysInit] StepperController FAILED");
    }
    g_stepperCtrl.disableAll();

    // 6. Wi-Fi (неблокирующий)
    Serial.println("[SysInit] WiFi init (non-blocking)...");
    g_wifi.begin();
    g_wifi.printStatus();

    // 7. Сеть (UDP сокеты) — ★ порты из конфига
    g_network.begin(g_config.network.telemetryPort, g_config.network.commandPort);
    Serial.println("[SysInit] NetworkManager OK");

    g_systemReady = true;
    Serial.println("[SysInit] === DONE ===");
    return g_systemReady;
}

// ============================================================
//  System_StartTasks — делегирование запуска задач
// ============================================================

void System_StartTasks() {
    if (!g_systemReady) return;

    auto& cfg = g_config.control;

    // Задача управления (Core 1, приоритет 3)
    xTaskCreatePinnedToCore(
        controlTask, "ControlTask", 4096, nullptr,
        cfg.controlTaskPriority, nullptr, cfg.controlTaskCore);

    // Задача датчиков (Core 1, приоритет 2)
    SensorManager::instance().startTask(cfg.sensorTaskCore, cfg.sensorTaskPriority);

    // Задача Wi-Fi (Core 0, приоритет 1)
    xTaskCreatePinnedToCore(
        wifiTask, "WifiTask", 4096, nullptr,
        cfg.wifiTaskPriority, nullptr, cfg.wifiTaskCore);

    // Задача сети (Core 0, приоритет 2)
    g_network.startTask(cfg.networkTaskCore, cfg.networkTaskPriority);

    Serial.println("[SysInit] All tasks started");
}

// ============================================================
//  Конфиг (одноразовый доступ)
// ============================================================

const ProjectConfig& System_GetConfig() { return g_config; }

uint32_t System_GetTelemetryPeriodMs() {
    return g_config.network.telemetryPeriodMs;
}

uint32_t System_GetControlPeriodMs() {
    return g_config.control.loopPeriodMs;
}

// ============================================================
//  Статус Wi-Fi (используется в main.cpp)
// ============================================================

bool System_WiFiConnected() {
    return g_wifi.isConnected();
}

// ============================================================
//  namespace sys
// ============================================================

namespace sys {
    bool initAll() { return System_Init(); }
    void startTasks() { System_StartTasks(); }
}