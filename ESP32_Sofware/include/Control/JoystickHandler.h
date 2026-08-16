#pragma once
#include <cstdint>
#include "Control/Orientation.h"

namespace control {

struct JoystickConfig {
    int pinX1 = 36, pinY1 = 39, pinX2 = 32, pinY2 = 33;
    int pinK1 = 34, pinK2 = 35, pinSma = 25;
    int rawMin = 0, rawCenter = 2048, rawMax = 4095;
    int   oversampling = 8;
    float emaAlpha = 0.35f;
    float deadzone = 0.08f;
    float expo = 2.2f;
    float maxYawRate = 60.f, maxPitchRate = 45.f;
    float maxRollRate = 90.f, maxBetaRate = 60.f;
    float fineModeFactor = 0.25f;
    uint32_t debounceMs = 20;
    uint32_t longPressMs = 600;   // порог долгого нажатия K1
};

struct JoystickState {
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    bool k1 = false, k2 = false, sma = false;
    bool k1Edge = false, k2Edge = false, smaEdge = false;
    bool k1ShortEdge = false;
    bool fineMode = false;
    float yawRate = 0, pitchRate = 0, rollRate = 0, betaRate = 0;
    
    // НОВЫЕ ПОЛЯ для управления
    int  controlMode   = 0;      // 0 = ручной, 1 = пресеты
    bool homeRequested = false;  // K1 долгое нажатие
};

class JoystickHandler {
public:
    void init(const JoystickConfig& cfg);
    void update();
    const JoystickState& state() const { return st_; }
    void clearHomeRequest() { st_.homeRequested = false; }

private:
    float readAxisRaw(int pin) const;
    float normalize(int raw) const;
    float shape(float v) const;

    JoystickConfig cfg_;
    JoystickState st_;
    float filt_[4] = {0, 0, 0, 0};
    
    struct BtnDeb { bool raw = false; bool stable = false; uint32_t lastChange = 0; };
    BtnDeb bK1_, bK2_, bSma_;
    bool lastK1_ = false, lastK2_ = false, lastSma_ = false;
    uint32_t k1PressStart_ = 0;
    bool k1LongFired_ = false;
    
    void updateButton(BtnDeb& b, bool rawNow, uint32_t now);
};

// Глобальный экземпляр
JoystickHandler& joystick();

} // namespace control