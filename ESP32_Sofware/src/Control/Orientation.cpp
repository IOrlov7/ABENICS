#include "Control/Orientation.h"

namespace control {

float wrapAngle(float a) {
    a = fmodf(a + kPi, 2.f * kPi);
    if (a < 0.f) a += 2.f * kPi;
    return a - kPi;
}

// ------------------------------------------------------ Euler <-> Quat (ZYZ)
// q = Rz(ψ) * Ry(θ) * Rz(φ)
Quaternion quatFromEuler(const YawPitchRoll& e) {
    const float cy = cosf(e.yaw * 0.5f),   sy = sinf(e.yaw * 0.5f);
    const float cp = cosf(e.pitch * 0.5f), sp = sinf(e.pitch * 0.5f);
    const float cr = cosf(e.roll * 0.5f),  sr = sinf(e.roll * 0.5f);
    return {
        cy * cp * cr - sy * cp * sr,   // w
        sp * (cy * sr - sy * cr),      // x
        sp * (cy * cr + sy * sr),      // y
        cp * (sy * cr + cy * sr)       // z
    };
}

YawPitchRoll eulerFromQuat(const Quaternion& qIn) {
    const Quaternion q = qIn.normalized();
    // Матрица поворота из кватерниона (используем только нужные элементы)
    const float r02 = 2.f * (q.x * q.z + q.w * q.y);
    const float r12 = 2.f * (q.y * q.z - q.w * q.x);
    const float r20 = 2.f * (q.x * q.z - q.w * q.y);
    const float r21 = 2.f * (q.y * q.z + q.w * q.x);
    const float r22 = 1.f - 2.f * (q.x * q.x + q.y * q.y);
    const float r00 = 1.f - 2.f * (q.y * q.y + q.z * q.z);
    const float r10 = 2.f * (q.x * q.y + q.w * q.z);

    YawPitchRoll e;
    const float sinTheta = sqrtf(r02 * r02 + r12 * r12);

    if (sinTheta < 1e-3f) {
        // GIMBAL LOCK: θ ≈ 0 или θ ≈ π, оси ψ и φ совпадают.
        // Догма: весь поворот отдаём в yaw, roll = 0.
        e.roll = 0.f;
        e.pitch = (r22 > 0.f) ? 0.f : kPi;
        e.yaw = (r22 > 0.f) ? atan2f(r10, r00)      // θ≈0:  ψ+φ -> ψ
                            : atan2f(-r10, -r00);   // θ≈π:  ψ-φ -> ψ
        return e;
    }
    e.pitch = atan2f(sinTheta, r22);      // θ ∈ (0, π)
    e.yaw   = atan2f(r12, r02);           // ψ
    e.roll  = atan2f(r21, -r20);          // φ
    return e;
}

Quaternion quatFromAxisAngle(const Vec3& axis, float angle) {
    const Vec3 a = axis.normalized();
    const float h = angle * 0.5f;
    const float s = sinf(h);
    return {cosf(h), a.x * s, a.y * s, a.z * s};
}

void quatToAxisAngle(const Quaternion& qIn, Vec3& axis, float& angle) {
    Quaternion q = qIn.normalized();
    if (q.w < 0.f) q = q.negated();               // всегда кратчайший путь
    angle = 2.f * acosf(clampf(q.w, -1.f, 1.f));
    const float s = sqrtf(fmaxf(0.f, 1.f - q.w * q.w));
    axis = (s > 1e-6f) ? Vec3{q.x / s, q.y / s, q.z / s} : Vec3{0, 0, 1};
}

Quaternion quatFromTwoVectors(const Vec3& from, const Vec3& to) {
    const Vec3 a = from.normalized(), b = to.normalized();
    const float d = a.dot(b);
    if (d > 0.9999f) return {1, 0, 0, 0};
    if (d < -0.9999f) {  // антипараллельны: ось произвольна перпендикулярная
        Vec3 t = fabsf(a.x) < 0.9f ? Vec3{1, 0, 0} : Vec3{0, 1, 0};
        return quatFromAxisAngle(a.cross(t).normalized(), kPi);
    }
    const Vec3 c = a.cross(b);
    return Quaternion{1.f + d, c.x, c.y, c.z}.normalized();
}

Vec3 quatRotate(const Quaternion& q, const Vec3& v) {
    // v' = q * v * q^-1, оптимизированная форма
    const Vec3 u{q.x, q.y, q.z};
    const Vec3 t = u.cross(v) * 2.f;
    return v + t * q.w + u.cross(t);
}

float angleBetweenQuat(const Quaternion& a, const Quaternion& b) {
    const float d = fabsf(a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z);
    return 2.f * acosf(clampf(d, 0.f, 1.f));
}

// ------------------------------------------------------ Интегрирование
Quaternion integrateBodyRate(const Quaternion& q, const Vec3& w, float dt) {
    // q̇ = 0.5 * q ⊗ (0, ω_body); первый порядок + ренормализация
    const Quaternion dq{1.f, w.x * 0.5f * dt, w.y * 0.5f * dt, w.z * 0.5f * dt};
    return (q * dq).normalized();
}

Quaternion integrateWorldRate(const Quaternion& q, const Vec3& w, float dt) {
    const Quaternion dq{1.f, w.x * 0.5f * dt, w.y * 0.5f * dt, w.z * 0.5f * dt};
    return (dq * q).normalized();
}

Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) {
    t = clampf(t, 0.f, 1.f);
    float dot = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
    Quaternion b2 = b;
    if (dot < 0.f) { dot = -dot; b2 = b.negated(); }   // кратчайшая дуга
    if (dot > 0.9995f) {                               // почти совпадают -> nlerp
        return Quaternion{
            a.w + (b2.w - a.w) * t, a.x + (b2.x - a.x) * t,
            a.y + (b2.y - a.y) * t, a.z + (b2.z - a.z) * t
        }.normalized();
    }
    const float th = acosf(dot);
    const float s  = sinf(th);
    const float ka = sinf((1.f - t) * th) / s;
    const float kb = sinf(t * th) / s;
    return Quaternion{
        a.w * ka + b2.w * kb, a.x * ka + b2.x * kb,
        a.y * ka + b2.y * kb, a.z * ka + b2.z * kb
    };
}

// ------------------------------------------------------ Скорости Эйлера <-> ω
// ω = ψ̇·Ẑ + θ̇·(Rz(ψ)·Ŷ) + φ̇·n, где n — продольная ось звена
Vec3 eulerRatesToOmega(const YawPitchRoll& e, const YawPitchRoll& r) {
    const float sy = sinf(e.yaw), cy = cosf(e.yaw);
    const float sp = sinf(e.pitch), cp = cosf(e.pitch);
    return {
        -r.pitch * sy + r.roll * sp * cy,
         r.pitch * cy + r.roll * sp * sy,
         r.yaw        + r.roll * cp
    };
}

bool omegaToEulerRates(const YawPitchRoll& e, const Vec3& w, YawPitchRoll& r) {
    const float sy = sinf(e.yaw), cy = cosf(e.yaw);
    const float sp = sinf(e.pitch), cp = cosf(e.pitch);
    r.pitch = -w.x * sy + w.y * cy;
    if (fabsf(sp) < 1e-3f) {
        // Gimbal Lock: ψ и φ неразделимы — всю ω_z отдаём в yaw
        r.roll = 0.f;
        r.yaw  = w.z;
        return false;
    }
    r.roll = (w.x * cy + w.y * sy) / sp;
    r.yaw  = w.z - r.roll * cp;
    return true;
}

} // namespace control