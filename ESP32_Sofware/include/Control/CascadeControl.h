#pragma once
#include "PID_Controller.h"

// Два каскада на каждую ось ABENICS
struct AxisCascade {
    PID_Controller outerPID;  // IMU -> target encoder angle
    PID_Controller innerPID;  // encoder angle -> motor command
    float targetImuAngle = 0.0f;
    float targetEncoderAngle = 0.0f;
    float motorOutput = 0.0f;
};

class CascadeControl {
public:
    static CascadeControl& GetInstance();

    void Init();
    void Update(float imuPitch, float imuRoll,
                float encAngleX, float encAngleY, float dt);

    AxisCascade& GetAxisX() { return _axisX; }
    AxisCascade& GetAxisY() { return _axisY; }

    void SetOuterCoeffsX(const PIDCoeffs& c);
    void SetInnerCoeffsX(const PIDCoeffs& c);
    void SetOuterCoeffsY(const PIDCoeffs& c);
    void SetInnerCoeffsY(const PIDCoeffs& c);
    void SetTarget(float pitch, float roll);

private:
    CascadeControl() = default;
    AxisCascade _axisX; // Pitch -> NEMA23 X (GPIO16/17)
    AxisCascade _axisY; // Roll  -> NEMA23 Y (GPIO13/14)
};