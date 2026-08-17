// include/Init/SystemInit.h

#pragma once

#include <Arduino.h>

#include "Init/ProjectConfig.h"
#include "Sensors/SensorManager.h"
#include "Communication/WiFiProvisioning.h"
#include "Communication/NetworkManager.h"
#include "Control/ManipulatorControl.h"
#include "Motors/Nema23/StepperController.h"
#include "Motors/TD7120MG/ServoController.h"

// --- Функции старого API (для совместимости или использования в main.cpp) ---
bool System_Init();
void System_StartTasks();

bool System_SetServoAngle(uint8_t channel, float angleDeg);
bool System_SetServoMicroseconds(uint8_t channel, uint16_t pulseUs);

bool System_EnableSteppers(bool enable);
bool System_MoveStepperSteps(uint8_t axis, int32_t steps, uint32_t stepDelayUs);

bool System_GetImuData(ImuData& out);

// <-- ДОБАВЛЕНО: Функция для получения статуса Wi-Fi из main.cpp -->
bool System_WiFiConnected();

// --- Новый API через namespace sys (для использования в других частях кода) ---
namespace sys {
    // Синглтоны всех систем
    WiFiProvisioning& wifi();
    NetworkManager& network();
    StepperController& steppers();
    ServoController& servos();
    
    // Единая точка инициализации (заменяет System_Init)
    bool initAll();
    
    // Запуск FreeRTOS задач (заменяет System_StartTasks)
    void startTasks();
}