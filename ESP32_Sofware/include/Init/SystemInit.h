#pragma once

#include <Arduino.h>

#include "Init/ProjectConfig.h"
#include "Sensors/SensorManager.h"

bool System_Init();
void System_StartTasks();

bool System_SetServoAngle(uint8_t channel, float angleDeg);
bool System_SetServoMicroseconds(uint8_t channel, uint16_t pulseUs);

bool System_EnableSteppers(bool enable);
bool System_MoveStepperSteps(uint8_t axis, int32_t steps, uint32_t stepDelayUs);

bool System_GetImuData(ImuData& out);