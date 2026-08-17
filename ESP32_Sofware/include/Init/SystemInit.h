#pragma once

#include <Arduino.h>
#include "Init/ProjectConfig.h"
#include "Communication/TelemetryPacket.h"

// ============================================================
//  SystemInit — ТОЛЬКО одноразовая инициализация при старте
// ============================================================

// --- Фасад инициализации ---
bool System_Init();
void System_StartTasks();

// --- Глобальные переменные состояния ---
extern volatile CommandId g_currentCommand;
extern volatile uint8_t   g_statusFlags;

// --- Конфиг (одноразовый доступ) ---
const ProjectConfig& System_GetConfig();
uint32_t System_GetTelemetryPeriodMs();
uint32_t System_GetControlPeriodMs();

// --- Статус Wi-Fi (используется в main.cpp) ---
bool System_WiFiConnected();

// --- Новый API через namespace sys ---
namespace sys {
    bool initAll();
    void startTasks();
}