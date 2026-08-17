#pragma once

#include <Arduino.h>
#include "Init/ProjectConfig.h"
#include "Communication/TelemetryPacket.h" // Для IMU_Data, CommandId

// Forward declarations для ускорения компиляции
class WiFiProvisioning;
class NetworkManager;
class StepperController;
class ServoController;

// --- Глобальные переменные состояния (объявления для других модулей) ---
extern volatile CommandId g_currentCommand;
extern volatile uint8_t g_statusFlags;

// --- Функции старого API (фасад для main.cpp и обратной совместимости) ---
bool System_Init();
void System_StartTasks();

bool System_SetServoAngle(uint8_t channel, float angleDeg);
bool System_SetServoMicroseconds(uint8_t channel, uint16_t pulseUs);
bool System_EnableSteppers(bool enable);
bool System_MoveStepperSteps(uint8_t axis, int32_t steps, uint32_t stepDelayUs);
bool System_GetImuData(IMU_Data& out); // ИСПРАВЛЕНО: IMU_Data вместо ImuData
bool System_WiFiConnected();

// --- Новый API через namespace sys ---
namespace sys {
    WiFiProvisioning& wifi();
    NetworkManager& network();
    StepperController& steppers();
    ServoController& servos();
    
    bool initAll();
    void startTasks();
}