#include "Motors/Nema23/StepperController.h"

bool StepperController::begin(const StepperConfig& cfg) {
    _cfg = cfg;

    if (_cfg.count > MAX_STEPPERS) {
        _cfg.count = MAX_STEPPERS;
    }

    for (uint8_t axis = 0; axis < _cfg.count; ++axis) {
        pinMode(_cfg.axes[axis].stepPin, OUTPUT);
        pinMode(_cfg.axes[axis].dirPin, OUTPUT);

        digitalWrite(_cfg.axes[axis].stepPin, HIGH);
        digitalWrite(_cfg.axes[axis].dirPin, LOW);

        _position[axis] = 0;
    }

    pinMode(_cfg.enablePin, OUTPUT);

    _enabled = false;
    disableAll();

    return true;
}

bool StepperController::enableAll() {
    if (_cfg.enableActiveLow) {
        digitalWrite(_cfg.enablePin, LOW);
    } else {
        digitalWrite(_cfg.enablePin, HIGH);
    }

    _enabled = true;
    return true;
}

bool StepperController::disableAll() {
    if (_cfg.enableActiveLow) {
        digitalWrite(_cfg.enablePin, HIGH);
    } else {
        digitalWrite(_cfg.enablePin, LOW);
    }

    _enabled = false;
    return true;
}

bool StepperController::isEnabled() const {
    return _enabled;
}

bool StepperController::isValidAxis(uint8_t axis) const {
    return (axis < _cfg.count && axis < MAX_STEPPERS);
}

bool StepperController::setDirection(uint8_t axis, bool forward) {
    if (!isValidAxis(axis)) {
        return false;
    }

    bool level = forward;

    if (_cfg.axes[axis].invertDirection) {
        level = !level;
    }

    digitalWrite(_cfg.axes[axis].dirPin, level ? HIGH : LOW);
    delayMicroseconds(2);

    return true;
}

void StepperController::pulseStep(uint8_t axis) {
    uint8_t stepPin = _cfg.axes[axis].stepPin;

    digitalWrite(stepPin, LOW);
    delayMicroseconds(2);

    digitalWrite(stepPin, HIGH);
    delayMicroseconds(2);
}

bool StepperController::moveSteps(uint8_t axis, int32_t steps, uint32_t stepDelayUs) {
    if (!isValidAxis(axis)) {
        return false;
    }

    if (steps == 0) {
        return true;
    }

    if (!_enabled) {
        enableAll();
    }

    bool forward = (steps > 0);

    if (!setDirection(axis, forward)) {
        return false;
    }

    int32_t stepsAbs = (steps > 0) ? steps : -steps;

    for (int32_t i = 0; i < stepsAbs; ++i) {
        pulseStep(axis);

        if (forward) {
            _position[axis] += 1;
        } else {
            _position[axis] -= 1;
        }

        if (stepDelayUs > 0) {
            delayMicroseconds(stepDelayUs);
        }

        if ((i & 0xFF) == 0) {
            yield();
        }
    }

    return true;
}

int32_t StepperController::getPosition(uint8_t axis) const {
    if (!isValidAxis(axis)) {
        return 0;
    }

    return _position[axis];
}

void StepperController::resetPosition(uint8_t axis, int32_t value) {
    if (!isValidAxis(axis)) {
        return;
    }

    _position[axis] = value;
}