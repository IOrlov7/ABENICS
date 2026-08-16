#include "Control/ManipulatorControl.h"
#include "Control/JoystickHandler.h"
#include "Init/ProjectConfig.h"
#include <Arduino.h>

// Подключаем драйверы моторов
#include "Motors/Nema23/StepperController.h"
#include "Motors/TD7120MG/ServoController.h"

// Глобальные экземпляры контроллеров (создаются в SystemInit)
extern StepperController g_stepperCtrl;
extern ServoController g_servoCtrl;

namespace control {


// Глобальный экземпляр IK (beta=180° по умолчанию)
static AbenicsMatrixIK g_ik(AbenicsMatrixIK::Config{});

// Таблица пресетов
const Preset kPresets[] = {
    {"HOME",     0.0f,  0.0f,  0.0f},
    {"GRAB",    30.0f, 20.0f,  0.0f},
    {"RELEASE",-30.0f, 20.0f, 45.0f},
    {"LOOK_UP",  0.0f, -30.0f, 0.0f},
};
const int kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);

// Параметры приводов
static const float SERVO_CENTER_DEG = 135.0f;  // середина 270° серво
static const float SERVO_RANGE_DEG  = 135.0f;  // ±135° от центра
static const float STEPS_PER_REV    = 200.0f;  // 1.8°/шаг
static const float STEPS_PER_RAD    = STEPS_PER_REV / (2.0f * kPi);
static const uint32_t STEP_DELAY_US = 500;     // задержка между шагами (подобрать!)

// Каналы PCA9685 для серво (из ProjectConfig)
static const uint8_t SERVO_CH_A = 0;  // thetaA1 (roll модуля A)
static const uint8_t SERVO_CH_B = 1;  // thetaB1 (roll модуля B)

// Текущие углы тангажа для шаговиков
static float g_thetaA2_current = 0.0f;
static float g_thetaB2_current = 0.0f;

// ------------------------------------------------------------------
// 1. Чтение датчиков
// ------------------------------------------------------------------
void readSensors(ManipulatorState& st) {
    st.encodersOk = false;

    // TODO: драйвер MT6816
    // st.motors.thetaA2 = mt6816X.readRad();
    // st.motors.thetaB2 = mt6816Y.readRad();
    // g_thetaA2_current = st.motors.thetaA2;
    // g_thetaB2_current = st.motors.thetaB2;
    // st.encodersOk = true;

    // Пока энкодеров нет — ориентация из FK модели
    SphericalOrientation o;
    g_ik.forward(st.motors, o);
    st.roll  = o.roll;
    st.pitch = o.pitch;
    st.yaw   = o.yaw;
}

// ------------------------------------------------------------------
// 2. Главная функция управления
// ------------------------------------------------------------------
void processControl(ManipulatorState& st, float dt) {
    JoystickHandler& js = joystick();
    const JoystickState& j = js.state();

    st.mode = j.controlMode;

    // K2 -> следующий пресет в режиме 1
    static bool lastK2 = false;
    if (j.k2 && !lastK2 && st.mode == 1) {
        st.activePreset = (st.activePreset + 1) % kPresetCount;
    }
    lastK2 = j.k2;

    // K1 долгое -> HOME
    if (j.homeRequested) {
        js.clearHomeRequest();
        st.activePreset = 0;
        st.mode = 1;
    }

    if (st.mode == 0) {
        // РУЧНОЙ РЕЖИМ
        float tRoll  = st.roll  + degToRad(j.rollRate)  * dt;
        float tPitch = st.pitch + degToRad(j.pitchRate) * dt;
        float tYaw   = st.yaw   + degToRad(j.yawRate)   * dt;

        tPitch = clampf(tPitch, -kPitchLimitRad, kPitchLimitRad);
        tRoll  = clampf(tRoll,  -kRollLimitRad,  kRollLimitRad);

        moveTowards(st, tRoll, tPitch, tYaw, 1.0f);
    } else {
        // РЕЖИМ ПРЕСЕТОВ
        applyPreset(st, st.activePreset);
    }
}

// ------------------------------------------------------------------
// 3. Движение к целевой ориентации
// ------------------------------------------------------------------
void moveTowards(ManipulatorState& st, float tRoll, float tPitch, float tYaw, float speed) {
    const float k = clampf(speed, 0.0f, 1.0f);
    st.roll  += (tRoll  - st.roll)  * k;
    st.pitch += (tPitch - st.pitch) * k;
    st.yaw   += (tYaw   - st.yaw)   * k;

    SphericalOrientation target;
    target.roll = st.roll;
    target.pitch = st.pitch;
    target.yaw = st.yaw;

    AbenicsAngles sol;
    g_ik.inverse(target, sol);
    st.motors = sol;

    // ================================================================
    // СЕРВОПРИВОДЫ (roll: thetaA1, thetaB1)
    // ================================================================
    float servoA_deg = radToDeg(sol.thetaA1) + SERVO_CENTER_DEG;
    float servoB_deg = radToDeg(sol.thetaB1) + SERVO_CENTER_DEG;
    
    servoA_deg = constrain(servoA_deg, 0.0f, 270.0f);
    servoB_deg = constrain(servoB_deg, 0.0f, 270.0f);

    g_servoCtrl.setServoAngle(SERVO_CH_A, servoA_deg);
    g_servoCtrl.setServoAngle(SERVO_CH_B, servoB_deg);

    // ================================================================
    // ШАГОВИКИ (pitch: thetaA2, thetaB2)
    // ================================================================
    float deltaA2 = sol.thetaA2 - g_thetaA2_current;
    float deltaB2 = sol.thetaB2 - g_thetaB2_current;

    int32_t stepsA = (int32_t)roundf(deltaA2 * STEPS_PER_RAD);
    int32_t stepsB = (int32_t)roundf(deltaB2 * STEPS_PER_RAD);

    if (stepsA != 0) {
        g_stepperCtrl.moveSteps(0, stepsA, STEP_DELAY_US);
        g_thetaA2_current = sol.thetaA2;
    }
    if (stepsB != 0) {
        g_stepperCtrl.moveSteps(1, stepsB, STEP_DELAY_US);
        g_thetaB2_current = sol.thetaB2;
    }
}

// ------------------------------------------------------------------
// 4. Применить пресет
// ------------------------------------------------------------------
void applyPreset(ManipulatorState& st, int index) {
    if (index < 0 || index >= kPresetCount) return;
    const Preset& p = kPresets[index];
    moveTowards(st, degToRad(p.rollDeg), degToRad(p.pitchDeg),
                degToRad(p.yawDeg), 0.15f);
}

} // namespace control