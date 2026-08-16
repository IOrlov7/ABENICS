// src/Control/JoystickHandler.cpp

#include "Control/JoystickHandler.h"
#include <Arduino.h>
#include <cmath>

// --- ВАЖНО: Подключаем Orientation.h для clampf, degToRad, kPi ---
#include "Control/Orientation.h"
// --- ВАЖНО: Подключаем ManipulatorControl.h для kPitchLimitRad, kRollLimitRad ---
// (или определяем их здесь как constexpr, если не хотим зависимостей)
#include "Control/ManipulatorControl.h"

namespace control {

// --- Глобальный экземпляр ---
static JoystickHandler g_joystick;

// --- Реализация функции-геттера ---
JoystickHandler& joystick() {
    return g_joystick;
}

// --- Вспомогательная функция для ограничения ---
// (если не хочешь зависеть от Orientation.h, раскомментируй и используй эту)
/*
template<typename T>
T clamp(T v, T lo, T hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
*/

// --- Реализация методов класса ---

void JoystickHandler::init(const JoystickConfig& cfg) {
    cfg_ = cfg;
    analogReadResolution(12);
    const int axes[4] = {cfg_.pinX1, cfg_.pinY1, cfg_.pinX2, cfg_.pinY2};
    for (int p : axes) {
        analogSetPinAttenuation(p, ADC_11db);
    }

    pinMode(cfg_.pinK1, INPUT);
    pinMode(cfg_.pinK2, INPUT);
    pinMode(cfg_.pinSma, INPUT_PULLUP);

    // Прогрев фильтров
    for (int i = 0; i < 4; ++i) {
        filt_[i] = cfg_.rawCenter;
    }
    for (int i = 0; i < 8; ++i) {
        update();
    }
    // Инициализируем состояние
    st_ = JoystickState{};
}

float JoystickHandler::readAxisRaw(int pin) const {
    long sum = 0;
    for (int i = 0; i < cfg_.oversampling; ++i) {
        sum += analogRead(pin);
    }
    return (float)sum / cfg_.oversampling;
}

float JoystickHandler::normalize(int raw) const {
    const float c = cfg_.rawCenter;
    float v = (raw < c) ? (raw - c) / fmaxf(1.f, c - cfg_.rawMin)
                        : (raw - c) / fmaxf(1.f, cfg_.rawMax - cfg_.rawMin);
    return clampf(v, -1.f, 1.f); // используем из Orientation.h
}

float JoystickHandler::shape(float v) const {
    const float a = fabsf(v);
    if (a < cfg_.deadzone) return 0.f;
    const float n = (a - cfg_.deadzone) / (1.f - cfg_.deadzone);
    // powf может быть тяжёлой функцией, но для джойстика приемлемо
    return copysignf(powf(n, cfg_.expo), v);
}

void JoystickHandler::updateButton(BtnDeb& b, bool rawNow, uint32_t now) {
    if (rawNow != b.raw) { b.raw = rawNow; b.lastChange = now; }
    if ((now - b.lastChange) >= cfg_.debounceMs && b.raw != b.stable) {
        b.stable = b.raw;
    }
}

void JoystickHandler::update() {
    // --- ОСИ ---
    const int pins[4] = {cfg_.pinX1, cfg_.pinY1, cfg_.pinX2, cfg_.pinY2};
    float n[4]; // нормализованные и обработанные оси [-1 .. 1]
    for (int i = 0; i < 4; ++i) {
        // 1. Oversampling
        float raw = readAxisRaw(pins[i]);
        // 2. EMA Filter
        filt_[i] += cfg_.emaAlpha * (raw - filt_[i]);
        // 3. Normalize [-1 .. 1]
        float norm = normalize((int)filt_[i]);
        // 4. Apply Deadzone + Expo
        n[i] = shape(norm);
    }
    st_.x1 = n[0]; st_.y1 = n[1]; st_.x2 = n[2]; st_.y2 = n[3];

    // --- КНОПКИ ---
    const uint32_t now = millis();
    updateButton(bK1_,  !digitalRead(cfg_.pinK1),  now); // активный LOW
    updateButton(bK2_,  !digitalRead(cfg_.pinK2),  now);
    updateButton(bSma_, !digitalRead(cfg_.pinSma), now);

    // Сброс фронтов
    st_.k1Edge = false; st_.k2Edge = false; st_.smaEdge = false;
    st_.k1ShortEdge = false;

    const bool k1 = bK1_.stable;
    const bool k2 = bK2_.stable;
    const bool sma = bSma_.stable;

    // --- Обработка K1 (долгое/короткое нажатие) ---
    if (k1 && !lastK1_) {
        k1PressStart_ = now;
        k1LongFired_ = false; // сброс флага
    }
    if (k1 && lastK1_ && !k1LongFired_ && (now - k1PressStart_) >= cfg_.longPressMs) {
        k1LongFired_ = true;
        st_.homeRequested = true; // устанавливаем флаг
    }
    if (!k1 && lastK1_) {
        if (!k1LongFired_) {
            st_.k1ShortEdge = true; // короткое нажатие
        }
        // Сброс флага долгого нажатия после отпускания
        k1LongFired_ = false;
    }

    // --- Обработка SMA (переключение режима) ---
    if (sma && !lastSma_) {
        st_.controlMode ^= 1; // переключить 0/1
        st_.smaEdge = true; // фронт для внешнего использования
    }

    // --- Обработка K2 (фронт) ---
    if (k2 && !lastK2_) {
        st_.k2Edge = true;
    }

    // Сохраняем текущее состояние для следующего цикла
    lastK1_ = k1; lastK2_ = k2; lastSma_ = sma;
    st_.k1 = k1; st_.k2 = k2; st_.sma = sma;

    // --- Обработка короткого нажатия K1 (например, для точного режима) ---
    if (st_.k1ShortEdge) {
        st_.fineMode = !st_.fineMode;
    }

    // --- Маппинг осей в скорости (с учётом точного режима) ---
    const float fine_factor = st_.fineMode ? cfg_.fineModeFactor : 1.0f;

    st_.yawRate   =  st_.x1 * cfg_.maxYawRate   * fine_factor;
    st_.pitchRate = -st_.y1 * cfg_.maxPitchRate * fine_factor; // инвертируем, если нужно (от себя = наклон)
    st_.rollRate  =  st_.x2 * cfg_.maxRollRate  * fine_factor;
    st_.betaRate  = -st_.y2 * cfg_.maxBetaRate  * fine_factor; // betaRate пока не используется в IK, но пусть будет
}

// --- Реализация методов, не являющихся частью управления манипулятором ---
// (например, для получения сырых значений, калибровки)
/*
float JoystickHandler::getRawValue(int pin) const {
    return readAxisRaw(pin);
}

float JoystickHandler::getNormalizedValue(int idx) const {
    if (idx < 0 || idx > 3) return 0.0f;
    return st_.values[idx]; // предполагается, что st_ хранит сырые нормализованные значения
}
*/

} // namespace control