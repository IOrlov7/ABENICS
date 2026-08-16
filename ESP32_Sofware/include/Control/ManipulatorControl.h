#pragma once
#include "Control/AbenicsMatrixIK.h"

namespace control {

// Всё состояние манипулятора
struct ManipulatorState {
    float roll = 0, pitch = 0, yaw = 0;     // текущая ориентация (рад)
    AbenicsAngles motors;                    // текущие углы моторов
    bool encodersOk = false;
    int  mode = 0;                           // 0 = ручной, 1 = пресеты
    int  activePreset = 0;
};

// Пресет — именованная целевая ориентация
struct Preset {
    const char* name;
    float rollDeg, pitchDeg, yawDeg;
};

// --- 4 функции управления ---
void readSensors(ManipulatorState& st);
void processControl(ManipulatorState& st, float dt);
void moveTowards(ManipulatorState& st, float tRoll, float tPitch, float tYaw, float speed);
void applyPreset(ManipulatorState& st, int index);

// Таблица пресетов
extern const Preset kPresets[];
extern const int    kPresetCount;

// Лимиты для 180-градусной версии (из статьи, Table 2)
constexpr float kPitchLimitRad = 35.0f * kPi / 180.0f;  // <= ПОМЕНЯЙ НА ЭТУ СТРОКУ
constexpr float kRollLimitRad  = 120.0f * kPi / 180.0f; // <= ПОМЕНЯЙ НА ЭТУ СТРОКУ
} // namespace control