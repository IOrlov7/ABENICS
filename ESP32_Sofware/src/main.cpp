#include <Arduino.h>
#include "Init/ProjectConfig.h"
#include "Control/JoystickHandler.h"
#include "Control/ManipulatorControl.h"
#include "Motors/Nema23/StepperController.h"
#include "Motors/TD7120MG/ServoController.h"

control::ManipulatorState g_manipState;
// Глобальные контроллеры (определение, не extern!)
StepperController g_stepperCtrl;
ServoController g_servoCtrl;

void controlTask(void* pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(20);
    const float dt = 0.02f;

    for (;;) {
        control::joystick().update();
        control::readSensors(g_manipState);
        control::processControl(g_manipState, dt);
        vTaskDelayUntil(&lastWakeTime, period);
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println(F("=== ABENICS Controller Starting ==="));

    // Получаем единый конфиг проекта
    ProjectConfig cfg = GetProjectConfig();

    // StepperController
    if (!g_stepperCtrl.begin(cfg.stepper)) {
        Serial.println(F("[ERROR] StepperController init failed!"));
        while(1) { delay(1000); }
    }
    Serial.println(F("[OK] StepperController initialized"));

    // ServoController
    if (!g_servoCtrl.begin(PCA9685_ADDRESS, PCA9685_FREQ_HZ, SERVO_MIN_US, SERVO_MAX_US)) {
        Serial.println(F("[ERROR] ServoController init failed!"));
        while(1) { delay(1000); }
    }
    Serial.println(F("[OK] ServoController initialized"));

    // Joystick
    control::joystick().init(cfg.joystick);
    Serial.println(F("[OK] Joystick initialized"));

    g_stepperCtrl.enableAll();
    Serial.println(F("[OK] Steppers enabled"));

    g_servoCtrl.setServoAngle(SERVO_CH_ROLL_A, 135.0f);
    g_servoCtrl.setServoAngle(SERVO_CH_ROLL_B, 135.0f);
    Serial.println(F("[OK] Servos at center"));

    if (xTaskCreate(controlTask, "ControlTask", 4096, NULL, 3, NULL) != pdPASS) {
        Serial.println(F("[ERROR] Failed to create ControlTask!"));
        while(1) { delay(1000); }
    }
    Serial.println(F("[OK] ControlTask started at 50 Hz"));
    Serial.println(F("=== Setup complete ==="));
}

void loop() {
    delay(1000);
}