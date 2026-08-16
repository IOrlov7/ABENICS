#pragma once
#include <cmath>

namespace control
{

    constexpr float kPi = 3.14159265358979f;
    constexpr float kDegToRad = kPi / 180.0f;
    constexpr float kRadToDeg = 180.0f / kPi;

    inline float degToRad(float d) { return d * kDegToRad; }
    inline float radToDeg(float r) { return r * kRadToDeg; }
    inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
    float wrapAngle(float a); // -> [-pi, pi]

    // ---------------------------------------------------------------- Vec3
    struct Vec3
    {
        float x, y, z;

        Vec3() : x(0.f), y(0.f), z(0.f) {}
        Vec3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}

        Vec3 operator+(const Vec3 &o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
        Vec3 operator-(const Vec3 &o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
        Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
        float dot(const Vec3 &o) const { return x * o.x + y * o.y + z * o.z; }
        Vec3 cross(const Vec3 &o) const
        {
            return Vec3(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x);
        }
        float norm() const { return sqrtf(dot(*this)); }
        Vec3 normalized() const
        {
            float n = norm();
            return n > 1e-9f ? (*this) * (1.f / n) : Vec3(0.f, 0.f, 0.f);
        }
    };

    // ---------------------------------------------------------------- Mat3
    struct Mat3
    {
        float m[3][3];

        Mat3()
        {
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    m[i][j] = 0.0f;
        }

        Mat3 operator*(const Mat3 &o) const
        {
            Mat3 res;
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    for (int k = 0; k < 3; ++k)
                        res.m[i][j] += m[i][k] * o.m[k][j];
            return res;
        }

        Vec3 operator*(const Vec3 &v) const
        {
            return Vec3(
                m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
                m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z);
        }

        Mat3 transposed() const
        {
            Mat3 res;
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    res.m[i][j] = m[j][i];
            return res;
        }

        static Mat3 rotX(float a)
        {
            Mat3 r;
            float c = cosf(a), s = sinf(a);
            r.m[0][0] = 1.0f;
            r.m[1][1] = c;
            r.m[1][2] = -s;
            r.m[2][1] = s;
            r.m[2][2] = c;
            return r;
        }

        static Mat3 rotY(float a)
        {
            Mat3 r;
            float c = cosf(a), s = sinf(a);
            r.m[0][0] = c;
            r.m[0][2] = s;
            r.m[1][1] = 1.0f;
            r.m[2][0] = -s;
            r.m[2][2] = c;
            return r;
        }

        static Mat3 rotZ(float a)
        {
            Mat3 r;
            float c = cosf(a), s = sinf(a);
            r.m[0][0] = c;
            r.m[0][1] = -s;
            r.m[1][0] = s;
            r.m[1][1] = c;
            r.m[2][2] = 1.0f;
            return r;
        }

        static Mat3 fromEulerXYZ(float r, float p, float y)
        {
            return rotX(r) * rotY(p) * rotZ(y);
        }

        void toEulerXYZ(float &r, float &p, float &y) const
        {
            p = asinf(clampf(m[0][2], -1.0f, 1.0f));
            if (fabsf(cosf(p)) > 1e-6f)
            {
                y = atan2f(-m[0][1], m[0][0]);
                r = atan2f(-m[1][2], m[2][2]);
            }
            else
            {
                y = atan2f(m[1][0], m[1][1]);
                r = 0.0f;
            }
        }
    };

    // ----------------------------------------------------------- Quaternion
    struct Quaternion
    {
        float w, x, y, z;

        Quaternion() : w(1.f), x(0.f), y(0.f), z(0.f) {}
        Quaternion(float ww, float xx, float yy, float zz) : w(ww), x(xx), y(yy), z(zz) {}

        Quaternion operator*(const Quaternion &o) const
        {
            return Quaternion(
                w * o.w - x * o.x - y * o.y - z * o.z,
                w * o.x + x * o.w + y * o.z - z * o.y,
                w * o.y - x * o.z + y * o.w + z * o.x,
                w * o.z + x * o.y - y * o.x + z * o.w);
        }
        Quaternion conjugate() const { return Quaternion(w, -x, -y, -z); }
        Quaternion negated() const { return Quaternion(-w, -x, -y, -z); }
        float normSq() const { return w * w + x * x + y * y + z * z; }
        Quaternion normalized() const
        {
            float n = sqrtf(normSq());
            if (n < 1e-9f)
                return Quaternion(1.f, 0.f, 0.f, 0.f);
            float inv = 1.f / n;
            return Quaternion(w * inv, x * inv, y * inv, z * inv);
        }
    };

    // Углы Эйлера, конвенция ZYZ (внутри — всегда радианы!)
    // yaw ψ  — вокруг глобальной Z (азимут)
    // pitch θ — вокруг Y (отклонение от вертикали), диапазон [0..pi]
    // roll φ  — вокруг собственной продольной оси звена
    struct YawPitchRoll
    {
        float yaw, pitch, roll;
        YawPitchRoll() : yaw(0.f), pitch(0.f), roll(0.f) {}
        YawPitchRoll(float y, float p, float r) : yaw(y), pitch(p), roll(r) {}
    };

    // ------------------------- Преобразования (учитывают Gimbal Lock при θ→0/π)
    Quaternion quatFromEuler(const YawPitchRoll &ypr);
    YawPitchRoll eulerFromQuat(const Quaternion &q);

    Quaternion quatFromAxisAngle(const Vec3 &axis, float angleRad);
    void quatToAxisAngle(const Quaternion &q, Vec3 &axis, float &angleRad);
    Quaternion quatFromTwoVectors(const Vec3 &from, const Vec3 &to); // удобно для IMU
    Vec3 quatRotate(const Quaternion &q, const Vec3 &v);
    float angleBetweenQuat(const Quaternion &a, const Quaternion &b);

    // ------------------------- Интегрирование и интерполяция
    // omegaBody  — угловая скорость в СВЯЗАННОЙ системе (гироскоп IMU)
    // omegaWorld — в ГЛОБАЛЬНОЙ системе (команды оператора)
    Quaternion integrateBodyRate(const Quaternion &q, const Vec3 &omegaBody, float dt);
    Quaternion integrateWorldRate(const Quaternion &q, const Vec3 &omegaWorld, float dt);
    Quaternion slerp(const Quaternion &a, const Quaternion &b, float t);

    // ------------------------- Скорости Эйлера <-> угловая скорость (ZYZ)
    // rates = {dψ/dt, dθ/dt, dφ/dt}. Нужно для маппинга джойстика в ω.
    Vec3 eulerRatesToOmega(const YawPitchRoll &ypr, const YawPitchRoll &rates);
    // Обратное преобразование; вернёт false в Gimbal Lock (разложение ψ/φ неоднозначно)
    bool omegaToEulerRates(const YawPitchRoll &ypr, const Vec3 &omega, YawPitchRoll &rates);

} // namespace control