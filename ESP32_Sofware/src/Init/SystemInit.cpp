#include "Init/SystemInit.h"

#include "HAL/I2CBus.h"
#include "Sensors/SensorManager.h"
#include "Motors/TD7120MG/ServoController.h"
#include "Motors/Nema23/StepperController.h" // <-- Критически важно!

static ProjectConfig g_config;

static ServoController g_servoController;
static StepperController g_stepperController; 

static bool g_systemReady = false;

static void SensorTask(void* parameter) {
    while (true) {
        SensorManager::instance().update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool System_Init() {
    g_config = GetProjectConfig();

    auto& i2c = I2CBus::instance();

    if (!i2c.begin(
        g_config.i2c.sda,
        g_config.i2c.scl,
        g_config.i2c.frequencyHz
    )) {
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

    if (g_servoController.begin(
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

    if (g_stepperController.begin(g_config.stepper)) {
        Serial.println("SystemInit: StepperController init OK");
    } else {
        Serial.println("SystemInit: StepperController init FAILED");
    }

    g_stepperController.disableAll();

    g_systemReady = true;
    return g_systemReady;
}

void System_StartTasks() {
    if (!g_systemReady) {
        return;
    }

    xTaskCreatePinnedToCore(
        SensorTask,
        "SensorTask",
        4096,
        nullptr,
        2,
        nullptr,
        1
    );
}

bool System_SetServoAngle(uint8_t channel, float angleDeg) {
    if (!g_systemReady) return false;
    return g_servoController.setServoAngle(channel, angleDeg);
}

bool System_SetServoMicroseconds(uint8_t channel, uint16_t pulseUs) {
    if (!g_systemReady) return false;
    return g_servoController.setServoMicroseconds(channel, pulseUs);
}

bool System_EnableSteppers(bool enable) {
    if (!g_systemReady) return false;

    if (enable) {
        return g_stepperController.enableAll();
    }
    return g_stepperController.disableAll();
}

bool System_MoveStepperSteps(uint8_t axis, int32_t steps, uint32_t stepDelayUs) {
    if (!g_systemReady) return false;
    return g_stepperController.moveSteps(axis, steps, stepDelayUs);
}

bool System_GetImuData(ImuData& out) {
    if (!g_systemReady) return false;
    return SensorManager::instance().getImu(out);
}