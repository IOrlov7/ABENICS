#pragma once
#include <Arduino.h>

struct PIDCoeffs {
    PIDCoeffs() = default;
    PIDCoeffs(float kp, float ki, float kd, float kff)
        : Kp(kp), Ki(ki), Kd(kd), Kff(kff) {}

    float Kp = 0.0f;
    float Ki = 0.0f;
    float Kd = 0.0f;
    float Kff = 0.0f;  // Feed-forward (из вашего найденного примера)
};

class PID_Controller {
public:
    PID_Controller();
    void Init(const PIDCoeffs& coeffs, float outMin, float outMax);

    float Compute(float setpoint, float measurement, float dt);
    void SetCoeffs(const PIDCoeffs& coeffs);
    PIDCoeffs GetCoeffs() const { return _coeffs; }
    void Reset();

private:
    PIDCoeffs _coeffs;
    float _outMin, _outMax;
    float _integral;
    float _prevError;
    float _filteredDeriv;
    bool _firstRun;
};