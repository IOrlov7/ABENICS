#include <Arduino.h>
#include "Init/SystemInit.h"
#include "Control/JoystickHandler.h"
#include "Control/ManipulatorControl.h"

control::ManipulatorState g_manipState;

// --- Задача управления: джойстик + моторы (50 Гц) ---
// Работает ВСЕГДА, независимо от Wi-Fi
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

// --- Задача сети: UDP телеметрия ---
// Если Wi-Fi не подключен — просто пропускает отправку
void networkTask(void* pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(50); // 20 Гц для сети достаточно

    for (;;) {
        if (System_WiFiConnected()) {
            // Отправка телеметрии по UDP
            // network.sendTelemetry(packet);
        }
        // Если Wi-Fi нет — просто спим, не блокируем ничего
        vTaskDelayUntil(&lastWakeTime, period);
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println(F("=== ABENICS Controller Starting ==="));

    // Инициализация ВСЕХ систем.
    // Wi-Fi не блокирует: если не подключился — поднимет AP в фоне.
    // Джойстик и моторы инициализируются ВСЕГДА.
    if (!System_Init()) {
        Serial.println(F("[FATAL] System init failed!"));
        while (1) { delay(1000); }
    }

    // Запуск всех задач FreeRTOS
    System_StartTasks();

    Serial.println(F("=== Setup complete. Joystick control ACTIVE. ==="));
}

void loop() {
    // Serial мониторинг (работает ВСЕГДА, даже без Wi-Fi)
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 's' || c == 'S') {
            // Печать статуса по команде из Serial
            Serial.println(F("=== System Status ==="));
            Serial.print(F("WiFi: "));
            Serial.println(System_WiFiConnected() ? F("Connected") : F("Not connected (joystick mode)"));
            // Здесь можно добавить печать состояния моторов, датчиков и т.д.
        }
    }
    delay(10);
}