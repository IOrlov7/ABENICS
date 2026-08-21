#include <Arduino.h>
#include "Init/SystemInit.h"
#include "Control/JoystickHandler.h"
#include "Control/ManipulatorControl.h"
#include "Communication/SerialPort.h"

// Глобальное состояние манипулятора (используется в controlTask)
control::ManipulatorState g_manipState;

// --- Задача управления: джойстик + моторы (50 Гц) ---
// Работает ВСЕГДА, независимо от Wi-Fi
// (Объявлена как extern в SystemInit.cpp для регистрации в FreeRTOS)
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
    delay(2000); // Задержка для стабильного старта Serial
    Serial.println(F("=== ABENICS Controller Starting ==="));
        // Инициализация COM-порта
    g_serial.begin(115200);

    // Инициализация ВСЕХ систем.
    // Wi-Fi не блокирует: если не подключился — поднимет AP в фоне.
    // Джойстик и моторы инициализируются ВСЕГДА.
    if (!System_Init()) {
        Serial.println(F("[FATAL] System init failed!"));
        while (1) { delay(1000); }
    }

    // Запуск всех задач FreeRTOS (включая networkTask из SystemInit.cpp)
    System_StartTasks();

    Serial.println(F("=== Setup complete. Joystick control ACTIVE. ==="));
}

void loop() {

       // Все задачи работают в FreeRTOS
    vTaskDelay(portMAX_DELAY);
    // Serial мониторинг (работает ВСЕГДА, даже без Wi-Fi)
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 's' || c == 'S') {
            // Печать статуса по команде из Serial
            Serial.println(F("=== System Status ==="));
            Serial.print(F("WiFi: "));
            Serial.println(System_WiFiConnected() ? F("Connected") : F("Not connected (joystick mode)"));
            
            // Дополнительная отладочная информация
            Serial.print(F("Uptime: "));
            Serial.print(millis() / 1000);
            Serial.println(F(" sec"));
        }
    }
    delay(10);
}