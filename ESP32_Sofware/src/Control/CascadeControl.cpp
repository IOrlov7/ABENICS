#include "Control/CascadeControl.h"

CascadeControl& CascadeControl::GetInstance() {
    static CascadeControl instance;
    return instance;
}

void CascadeControl::Init() {
    // Начальные коэффициенты из ProjectConfig (или захардкод для старта)
    PIDCoeffs outerX = {1.5f, 0.1f, 0.05f, 0.0f};
    PIDCoeffs innerX = {2.0f, 0.5f, 0.0f,  0.3f};
    PIDCoeffs outerY = {1.5f, 0.1f, 0.05f, 0.0f};
    PIDCoeffs innerY = {2.0f, 0.5f, 0.0f,  0.3f};

    _axisX.outerPID.Init(outerX, -45.0f, 45.0f);  // выход: целевой угол энкодера
    _axisX.innerPID.Init(innerX, -2000.0f, 2000.0f); // выход: шаги/сек
    _axisY.outerPID.Init(outerY, -45.0f, 45.0f);
    _axisY.innerPID.Init(innerY, -2000.0f, 2000.0f);
}

void CascadeControl::Update(float imuPitch, float imuRoll,
                             float encAngleX, float encAngleY, float dt) {
    // Каскад X: IMU Pitch -> Encoder X -> Motor X
    _axisX.targetEncoderAngle = _axisX.outerPID.Compute(
        _axisX.targetImuAngle, imuPitch, dt);
    _axisX.motorOutput = _axisX.innerPID.Compute(
        _axisX.targetEncoderAngle, encAngleX, dt);

    // Каскад Y: IMU Roll -> Encoder Y -> Motor Y
    _axisY.targetEncoderAngle = _axisY.outerPID.Compute(
        _axisY.targetImuAngle, imuRoll, dt);
    _axisY.motorOutput = _axisY.innerPID.Compute(
        _axisY.targetEncoderAngle, encAngleY, dt);
}

void CascadeControl::SetTarget(float pitch, float roll) {
    _axisX.targetImuAngle = pitch;
    _axisY.targetImuAngle = roll;
}

void CascadeControl::SetOuterCoeffsX(const PIDCoeffs& c) { _axisX.outerPID.SetCoeffs(c); }
void CascadeControl::SetInnerCoeffsX(const PIDCoeffs& c) { _axisX.innerPID.SetCoeffs(c); }
void CascadeControl::SetOuterCoeffsY(const PIDCoeffs& c) { _axisY.outerPID.SetCoeffs(c); }
void CascadeControl::SetInnerCoeffsY(const PIDCoeffs& c) { _axisY.innerPID.SetCoeffs(c); }