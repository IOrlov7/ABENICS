#pragma once
#include "Control/Orientation.h"

namespace control {

struct AbenicsAngles {
    float thetaA1, thetaA2, thetaB1, thetaB2;   // rollA, pitchA, rollB, pitchB
    AbenicsAngles() : thetaA1(0), thetaA2(0), thetaB1(0), thetaB2(0) {}
};

struct SphericalOrientation {
    float roll, pitch, yaw;                     // XYZ Euler angles
    SphericalOrientation() : roll(0), pitch(0), yaw(0) {}
};

class AbenicsMatrixIK {
public:
    struct Config {
        // Механические константы расположения модулей (соосные валы, 180 град)
        float alphaA =  kPi / 2.0f;
        float betaA  =  0.0f;
        float gammaA = -kPi / 4.0f;
        
        float alphaB = -kPi / 2.0f;
        float betaB  =  0.0f;
        float gammaB =  3.0f * kPi / 4.0f;
        
        // Передаточное отношение (CS 32T / MP 16T)
        float gearRatio = 2.0f; 
    };

    explicit AbenicsMatrixIK(const Config& cfg);

    // Прямая кинематика (для отладки и round-trip)
    void forward(const AbenicsAngles& a, SphericalOrientation& out) const;
    
    // Обратная кинематика (целевая ориентация -> углы моторов)
    void inverse(const SphericalOrientation& o, AbenicsAngles& out) const;

private:
    Config cfg_;
    Mat3 BqM_A_, BqM_B_;
    Mat3 invBqM_A_, invBqM_B_;
    
    // Для сглаживания сингулярности (сохраняем предыдущий roll)
    mutable float prev_thetaA1_ = 0.0f;
    mutable float prev_thetaB1_ = 0.0f;
};

// Тест самосогласованности
bool matrixIKRoundTripTest(const AbenicsMatrixIK& ik, float& maxErrRad);

} // namespace control