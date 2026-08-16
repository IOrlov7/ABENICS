#include <math.h>
#include "Sensors/MPU6500/MPU6500_Handler.h"
#include "Sensors/MPU6500/MPU6500_Orientation_Handler.h"

// ═══ Встроенный класс Madgwick (отдаёт сырой кватернион) ═══
class SimpleMadgwick {
private:
    float q0, q1, q2, q3;
    float beta, invSampleFreq;
public:
    SimpleMadgwick() : q0(1), q1(0), q2(0), q3(0), beta(0.1f), invSampleFreq(0.01f) {}
    
    void begin(float sampleFreq, float _beta = 0.1f) {
        invSampleFreq = 1.0f / sampleFreq;
        beta = _beta;
        q0 = 1; q1 = 0; q2 = 0; q3 = 0;
    }
    
    // ВАЖНО: gx, gy, gz должны быть в RAD/S!
    void updateIMU(float gx, float gy, float gz, float ax, float ay, float az) {
        float recipNorm;
        float s0, s1, s2, s3;
        float qDot1, qDot2, qDot3, qDot4;
        float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2, _8q1, _8q2, q0q0, q1q1, q2q2, q3q3;

        qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
        qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
        qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
        qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

        if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
            recipNorm = invSqrt(ax * ax + ay * ay + az * az);
            ax *= recipNorm; ay *= recipNorm; az *= recipNorm;

            _2q0 = 2.0f * q0; _2q1 = 2.0f * q1; _2q2 = 2.0f * q2; _2q3 = 2.0f * q3;
            _4q0 = 4.0f * q0; _4q1 = 4.0f * q1; _4q2 = 4.0f * q2;
            _8q1 = 8.0f * q1; _8q2 = 8.0f * q2;
            q0q0 = q0 * q0; q1q1 = q1 * q1; q2q2 = q2 * q2; q3q3 = q3 * q3;

            s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
            s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
            s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
            s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;
            recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
            s0 *= recipNorm; s1 *= recipNorm; s2 *= recipNorm; s3 *= recipNorm;

            qDot1 -= beta * s0; qDot2 -= beta * s1; qDot3 -= beta * s2; qDot4 -= beta * s3;
        }

        q0 += qDot1 * invSampleFreq; q1 += qDot2 * invSampleFreq;
        q2 += qDot3 * invSampleFreq; q3 += qDot4 * invSampleFreq;

        recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
        q0 *= recipNorm; q1 *= recipNorm; q2 *= recipNorm; q3 *= recipNorm;
    }

    float getQuatW() { return q0; }
    float getQuatX() { return q1; }
    float getQuatY() { return q2; }
    float getQuatZ() { return q3; }
    
    float getRoll()  { return atan2f(2.0f*(q0*q1 + q2*q3), 1.0f - 2.0f*(q1*q1 + q2*q2)) * 57.29577951f; }
    float getPitch() { return asinf(2.0f*(q0*q2 - q3*q1)) * 57.29577951f; }
    float getYaw()   { return atan2f(2.0f*(q0*q3 + q1*q2), 1.0f - 2.0f*(q2*q2 + q3*q3)) * 57.29577951f; }

private:
    float invSqrt(float x) {
        float halfx = 0.5f * x; float y = x;
        long i = *(long*)&y; i = 0x5f3759df - (i>>1); y = *(float*)&i;
        y = y * (1.5f - (halfx * y * y)); return y;
    }
};

static SimpleMadgwick myFilter;
static unsigned long lastUpdateTime = 0;

Orientation_Handler::Orientation_Handler(float freq) : sampleFreq(freq) {}

void Orientation_Handler::begin() {
    myFilter.begin(sampleFreq, 0.1f); // 0.1 - beta (баланс между шумом и дрейфом)
    lastUpdateTime = micros();
}

bool Orientation_Handler::updateOrientation() {
    if (l_mpuData.isError || !g_mpuData.isCalibrated) return false;

    unsigned long currentTime = micros();
    float actualDt = (currentTime - lastUpdateTime) / 1000000.0f;
    lastUpdateTime = currentTime;

    if (actualDt < 0.005f) actualDt = 0.005f;
    if (actualDt > 0.05f) actualDt = 0.05f;

    // Передаём гироскоп в RAD/S (как требует алгоритм)
    myFilter.updateIMU(l_mpuData.gyroX, l_mpuData.gyroY, l_mpuData.gyroZ,
                       l_mpuData.accelX, l_mpuData.accelY, l_mpuData.accelZ);

    // Берём кватернион НАПРЯМУЮ (без пересчёта из углов!)
    l_mpuData.quatW = myFilter.getQuatW();
    l_mpuData.quatX = myFilter.getQuatX();
    l_mpuData.quatY = myFilter.getQuatY();
    l_mpuData.quatZ = myFilter.getQuatZ();

    // Углы Эйлера теперь просто для отладки в логе
    l_mpuData.roll  = myFilter.getRoll();
    l_mpuData.pitch = myFilter.getPitch();
    l_mpuData.yaw   = myFilter.getYaw();

    return true;
}