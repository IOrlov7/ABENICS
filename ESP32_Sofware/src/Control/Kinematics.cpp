#include "Control/Kinematics.h"

namespace control {

Vec3 Kinematics::forward(const YawPitchRoll& ori, float beta) const {
    // В связанной системе: локоть = (0,0,L1) + Ry(β)·(0,0,L2)
    const Vec3 local{cfg_.l2 * sinf(beta), 0.f,
                     cfg_.l1 + cfg_.l2 * cosf(beta)};
    return quatRotate(quatFromEuler(ori), local);
}

IkResult Kinematics::inverse(const Vec3& target, float yawHint, float betaSign) const {
    IkResult res;
    const float l1 = cfg_.l1, l2 = cfg_.l2;
    const float rMin = fabsf(l1 - l2) + 1.f;   // мм, зазор от вырождения
    const float rMax = l1 + l2 - 1.f;

    const float r = target.norm();
    if (r < 1e-3f) {                            // цель в основании — недостижимо
        res.achieved = forward({yawHint, 0.f, 0.f}, 0.f);
        return res;
    }
    const float rC = clampf(r, rMin, rMax);
    res.clamped = fabsf(rC - r) > 0.5f;

    // 1) Beta из теоремы косинусов
    float cosBeta = (rC * rC - l1 * l1 - l2 * l2) / (2.f * l1 * l2);
    float beta = betaSign * acosf(clampf(cosBeta, -1.f, 1.f));
    beta = clampf(beta, cfg_.betaMin, cfg_.betaMax);

    // 2) Эффективный радиус после ограничения beta
    const float rEff = sqrtf(l1 * l1 + l2 * l2 + 2.f * l1 * l2 * cosf(beta));

    // 3) Yaw = азимут цели (на оси Z — сохраняем предыдущий)
    const float rho = sqrtf(target.x * target.x + target.y * target.y);
    const float yaw = (rho > 1e-3f) ? atan2f(target.y, target.x) : yawHint;

    // 4) Pitch: ρ = A·sinθ + B·cosθ, z = A·cosθ − B·sinθ
    //    => θ = atan2(ρ, z) − atan2(B, A)
    const float scale = rEff / r;               // направление цели, радиус rEff
    const float A = l1 + l2 * cosf(beta);
    const float B = l2 * sinf(beta);
    float pitch = atan2f(rho * scale, target.z * scale) - atan2f(B, A);

    // ZYZ: θ ∈ [0, π]. Отрицательный наклон ≡ положительный + yaw на π
    if (pitch < 0.f) { pitch = -pitch; }
    pitch = clampf(pitch, 0.f, cfg_.pitchMax);

    res.orientation.yaw   = (pitch < 1e-6f && rho < 1e-3f) ? yawHint : yaw;
    res.orientation.pitch = pitch;
    res.orientation.roll  = 0.f;
    res.beta = beta;
    res.achieved = forward(res.orientation, beta);

    const float dx = res.achieved.x - target.x;
    const float dy = res.achieved.y - target.y;
    const float dz = res.achieved.z - target.z;
    res.reachable = !res.clamped && sqrtf(dx*dx + dy*dy + dz*dz) < 2.f; // 2 мм
    return res;
}

} // namespace control