#pragma once

#include <Arduino.h>

// КРИТИЧНО ВАЖНО: Включаем конфиг, чтобы был известен тип StepperConfig
#include "Init/ProjectConfig.h" 

class StepperController {
public:
    bool begin(const StepperConfig& cfg);

    bool enableAll();
    bool disableAll();

    bool isEnabled() const;

    bool setDirection(uint8_t axis, bool forward);

    // steps > 0 -> вперёд
    // steps < 0 -> назад
    bool moveSteps(uint8_t axis, int32_t steps, uint32_t stepDelayUs);

    int32_t getPosition(uint8_t axis) const;
    void resetPosition(uint8_t axis, int32_t value = 0);

private:
    static constexpr uint8_t MAX_STEPPERS = 2;

    bool isValidAxis(uint8_t axis) const;
    void pulseStep(uint8_t axis);

    StepperConfig _cfg;

    bool _enabled = false;

    int32_t _position[MAX_STEPPERS] = {0, 0};
};