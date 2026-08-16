#pragma once
#include "Control/Orientation.h"

namespace control {

struct IkResult {
    bool reachable = false;       // цель достижима без ограничений
    bool clamped   = false;       // пришлось ограничивать (радиус/углы)
    YawPitchRoll orientation;     // ψ, θ (roll = 0)
    float beta = 0.f;             // изгиб L2, рад
    Vec3 achieved;                // реально достижимая точка
};

class Kinematics {
public:
    struct Config {
        float l1 = 300.f;                       // мм
        float l2 = 200.f;                       // мм
        float betaMin = degToRad(-90.f);        // лимиты узла L2
        float betaMax = degToRad(120.f);
        float pitchMax = degToRad(60.f);        // мех. упор шарнира
    };

    explicit Kinematics(const Config& cfg) : cfg_(cfg) {}

    // Прямая кинематика: ориентация L1 + beta -> точка кончика
    Vec3 forward(const YawPitchRoll& ori, float beta) const;

    // Обратная: точка -> {yaw, pitch, beta}; roll = 0.
    // betaSign: +1 / -1 — выбор «локтя».
    // yawHint: предыдущий yaw (цель на оси Z -> азимут не определён).
    IkResult inverse(const Vec3& target, float yawHint = 0.f,
                     float betaSign = +1.f) const;

private:
    Config cfg_;
};

} // namespace control