#include "Init/SystemInit.h"

#include "HAL/I2CBus.h"
#include "Sensors/SensorManager.h"
#include "Motors/TD7120MG/ServoController.h"
#include "Motors/Nema23/StepperController.h"
#include "Communication/WiFiProvisioning.h"
#include "Communication/NetworkManager.h"

static ProjectConfig g_config;

// ← ИСПРАВЛЕНО: убран static, имена совпадают с extern в ManipulatorControl
ServoController g_servoCtrl;
StepperController g_stepperCtrl;

static WiFiProvisioning g_wifi;
static NetworkManager g_network;

static bool g_systemReady = false;

extern void controlTask(void* parameter);
extern void networkTask(void* parameter);

static void wifiTask(void* parameter) {
    for (;;) {
        g_wifi.handle();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void sensorTask(void* parameter) {
    while (true) {
        SensorManager::instance().update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool System_Init() {
    g_config = GetProjectConfig();

    auto& i2c = I2CBus::instance();
    if (!i2c.begin(g_config.i2c.sda, g_config.i2c.scl, g_config.i2c.frequencyHz)) {
        Serial.println("SystemInit: I2C init FAILED");
        return false;
    }
    Serial.println("SystemInit: I2C init OK");
    i2c.scan();

    if (SensorManager::instance().begin(g_config)) {
        Serial.println("SystemInit: SensorManager init OK");
    } else {
        Serial.println("SystemInit: SensorManager init FAILED");
    }

    // ← ИСПРАВЛЕНО: используем g_servoCtrl вместо g_servoController
    if (g_servoCtrl.begin(
        g_config.servo.pca9685Address,
        g_config.servo.frequencyHz,
        g_config.servo.minUs,
        g_config.servo.maxUs,
        g_config.servo.count
    )) {
        Serial.println("SystemInit: ServoController init OK");
    } else {
        Serial.println("SystemInit: ServoController init FAILED");
    }

    // ← ИСПРАВЛЕНО: используем g_stepperCtrl вместо g_stepperController
    if (g_stepperCtrl.begin(g_config.stepper)) {
        Serial.println("SystemInit: StepperController init OK");
    } else {
        Serial.println("SystemInit: StepperController init FAILED");
    }
    g_stepperCtrl.disableAll();

    Serial.println("SystemInit: WiFi init (non-blocking)...");
    g_wifi.begin();
    g_wifi.printStatus();

    g_network.begin();
    Serial.println("SystemInit: NetworkManager init OK");

    g_systemReady = true;
    return g_systemReady;
}

void System_StartTasks() {
    if (!g_systemReady) return;

    xTaskCreatePinnedToCore(controlTask, "ControlTask", 4096, nullptr, 3, nullptr, 1);
    xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, nullptr, 2, nullptr, 1);
    xTaskCreatePinnedToCore(wifiTask, "WifiTask", 4096, nullptr, 1, nullptr, 0);
    xTaskCreatePinnedToCore(networkTask, "NetworkTask", 4096, nullptr, 2, nullptr, 0);

    Serial.println("SystemInit: All tasks started");
}

bool System_SetServoAngle(uint8_t channel, float angleDeg) {
    if (!g_systemReady) return false;
    return g_servoCtrl.setServoAngle(channel, angleDeg);  // ← g_servoCtrl
}

bool System_SetServoMicroseconds(uint8_t channel, uint16_t pulseUs) {
    if (!g_systemReady) return false;
    return g_servoCtrl.setServoMicroseconds(channel, pulseUs);  // ← g_servoCtrl
}

bool System_EnableSteppers(bool enable) {
    if (!g_systemReady) return false;
    if (enable) return g_stepperCtrl.enableAll();   // ← g_stepperCtrl
    return g_stepperCtrl.disableAll();               // ← g_stepperCtrl
}

bool System_MoveStepperSteps(uint8_t axis, int32_t steps, uint32_t stepDelayUs) {
    if (!g_systemReady) return false;
    return g_stepperCtrl.moveSteps(axis, steps, stepDelayUs);  // ← g_stepperCtrl
}

bool System_GetImuData(ImuData& out) {
    if (!g_systemReady) return false;
    return SensorManager::instance().getImu(out);
}

bool System_WiFiConnected() {
    return g_wifi.isConnected();
}