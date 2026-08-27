#include "Control/PID_Controller.h"

PID_Controller::PID_Controller()
    : _outMin(-1000), _outMax(1000), _integral(0),
      _prevError(0), _filteredDeriv(0), _firstRun(true) {}

void PID_Controller::Init(const PIDCoeffs& coeffs, float outMin, float outMax) {
    _coeffs = coeffs;
    _outMin = outMin;
    _outMax = outMax;
    Reset();
}

float PID_Controller::Compute(float setpoint, float measurement, float dt) {
    if (dt <= 0.0f) return 0.0f;
    float error = setpoint - measurement;

    // P
    float P = _coeffs.Kp * error;

    // I с anti-windup
    _integral += error * dt;
    float iMax = (_outMax - P) / (_coeffs.Ki + 1e-6f);
    float iMin = (_outMin - P) / (_coeffs.Ki + 1e-6f);
    _integral = constrain(_integral, iMin, iMax);
    float I = _coeffs.Ki * _integral;

    // D с ФНЧ (alpha = 0.1)
    float D = 0.0f;
    if (!_firstRun) {
        float rawD = (error - _prevError) / dt;
        _filteredDeriv = 0.1f * rawD + 0.9f * _filteredDeriv;
        D = _coeffs.Kd * _filteredDeriv;
    }
    _firstRun = false;
    _prevError = error;

    // FF (feed-forward)
    float FF = _coeffs.Kff * setpoint;

    return constrain(P + I + D + FF, _outMin, _outMax);
}

void PID_Controller::SetCoeffs(const PIDCoeffs& c) { _coeffs = c; }
void PID_Controller::Reset() {
    _integral = 0; _prevError = 0; _filteredDeriv = 0; _firstRun = true;
}