#include "Control/AbenicsMatrixIK.h"
#include <Arduino.h>

namespace control {

AbenicsMatrixIK::AbenicsMatrixIK(const Config& cfg) : cfg_(cfg) {
    BqM_A_ = Mat3::rotX(cfg_.alphaA) * Mat3::rotY(cfg_.betaA) * Mat3::rotZ(cfg_.gammaA);
    BqM_B_ = Mat3::rotX(cfg_.alphaB) * Mat3::rotY(cfg_.betaB) * Mat3::rotZ(cfg_.gammaB);
    invBqM_A_ = BqM_A_.transposed();
    invBqM_B_ = BqM_B_.transposed();
}

void AbenicsMatrixIK::inverse(const SphericalOrientation& o, AbenicsAngles& out) const {
    Mat3 BqH = Mat3::fromEulerXYZ(o.roll, o.pitch, o.yaw);

    // --- Модуль A ---
    Vec3 J_hat_A(1.0f, 0.0f, 0.0f);
    Vec3 J_A = invBqM_A_ * (BqH * J_hat_A);
    
    // Обработка сингулярности (полюс)
    if (fabsf(J_A.x) > 0.9999f) {
        out.thetaA1 = prev_thetaA1_; 
    } else {
        out.thetaA1 = atan2f(J_A.y, J_A.z);
        prev_thetaA1_ = out.thetaA1;
    }
    // Умножаем на gearRatio (2.0), так как у MP шестерни в 2 раза меньше зубьев
    out.thetaA2 = cfg_.gearRatio * acosf(clampf(J_A.x, -1.0f, 1.0f));

    // --- Модуль B ---
    Vec3 J_hat_B(0.0f, 1.0f, 0.0f);
    Vec3 J_B = invBqM_B_ * (BqH * J_hat_B);
    
    if (fabsf(J_B.x) > 0.9999f) {
        out.thetaB1 = prev_thetaB1_;
    } else {
        out.thetaB1 = atan2f(J_B.y, J_B.z);
        prev_thetaB1_ = out.thetaB1;
    }
    out.thetaB2 = cfg_.gearRatio * acosf(clampf(J_B.x, -1.0f, 1.0f));
}

void AbenicsMatrixIK::forward(const AbenicsAngles& a, SphericalOrientation& out) const {
    // Восстанавливаем вектор полюса A в системе модуля A
    float half_pA = a.thetaA2 / cfg_.gearRatio;
    // ИСПРАВЛЕНО: sin(thetaA1) для Y, cos(thetaA1) для Z
    Vec3 J_A(cosf(half_pA), sinf(half_pA) * sinf(a.thetaA1), sinf(half_pA) * cosf(a.thetaA1));
    Vec3 col0 = BqM_A_ * J_A; // Это первый столбец матрицы BqH

    // Восстанавливаем вектор полюса B в системе модуля B
    float half_pB = a.thetaB2 / cfg_.gearRatio;
    // ИСПРАВЛЕНО: sin(thetaB1) для Y, cos(thetaB1) для Z
    Vec3 J_B(cosf(half_pB), sinf(half_pB) * sinf(a.thetaB1), sinf(half_pB) * cosf(a.thetaB1));
    Vec3 col1 = BqM_B_ * J_B; // Это второй столбец матрицы BqH

    // Третий столбец - векторное произведение первых двух
    Vec3 col2 = col0.cross(col1);

    Mat3 BqH;
    BqH.m[0][0] = col0.x; BqH.m[1][0] = col0.y; BqH.m[2][0] = col0.z;
    BqH.m[0][1] = col1.x; BqH.m[1][1] = col1.y; BqH.m[2][1] = col1.z;
    BqH.m[0][2] = col2.x; BqH.m[1][2] = col2.y; BqH.m[2][2] = col2.z;

    BqH.toEulerXYZ(out.roll, out.pitch, out.yaw);
}

bool matrixIKRoundTripTest(const AbenicsMatrixIK& ik, float& maxErrRad) {
    static const float probe[4][3] = {
        {degToRad(30), degToRad(40), degToRad(50)},
        {degToRad(-20),degToRad(25), degToRad(60)},
        {degToRad(80), degToRad(15), degToRad(120)},
        {degToRad(10), degToRad(45), degToRad(30)},
    };
    maxErrRad = 0.f;
    Serial.println(F("=== MATRIX IK round-trip FK->IK->FK ==="));
    for (int i = 0; i < 4; ++i) {
        SphericalOrientation o;
        o.roll = probe[i][0]; o.pitch = probe[i][1]; o.yaw = probe[i][2];

        AbenicsAngles a;
        ik.inverse(o, a);

        SphericalOrientation o2;
        ik.forward(a, o2);

        const float e = fabsf(wrapAngle(o.roll  - o2.roll))
                      + fabsf(wrapAngle(o.pitch - o2.pitch))
                      + fabsf(wrapAngle(o.yaw   - o2.yaw));
        if (e > maxErrRad) maxErrRad = e;

        Serial.printf("probe %d: target=(%.1f,%.1f,%.1f) reFK=(%.2f,%.2f,%.2f) err=%.4f deg\n",
            i,
            radToDeg(o.roll), radToDeg(o.pitch), radToDeg(o.yaw),
            radToDeg(o2.roll), radToDeg(o2.pitch), radToDeg(o2.yaw),
            radToDeg(e));
    }
    const bool ok = maxErrRad < degToRad(0.1f);
    Serial.printf("max err = %.4f deg -> %s\n", radToDeg(maxErrRad), ok ? "OK" : "FAIL");
    return ok;
}

} // namespace control