#pragma once
#include <cstdint>
#include "Control/JoystickHandler.h"

// ============================================================
// ПИНЫ ИЗ АРХИТЕКТУРЫ (архитектура abenics.txt)
// ============================================================

// --- Геймпад (YWRobot, питание строго 3.3V) ---
constexpr int PIN_JOY_X1 = 36;  // ADC1_CH0
constexpr int PIN_JOY_Y1 = 39;  // ADC1_CH1
constexpr int PIN_JOY_X2 = 32;  // ADC1_CH4
constexpr int PIN_JOY_Y2 = 33;  // ADC1_CH5
constexpr int PIN_JOY_K1 = 34;  // внешний pull-up 10k (догма!)
constexpr int PIN_JOY_K2 = 35;  // внешний pull-up 10k (догма!)
constexpr int PIN_JOY_SMA = 25; // внутренний/внешний pull-up

// --- Шаговые двигатели (драйверы B1206) ---
constexpr int PIN_STEP_X = 16; // B1206 #1 PUL-
constexpr int PIN_DIR_X = 17;  // B1206 #1 DIR-
constexpr int PIN_STEP_Y = 13; // B1206 #2 PUL-
constexpr int PIN_DIR_Y = 14;  // B1206 #2 DIR-
constexpr int PIN_EN_ALL = 4;  // B1206 #1 & #2 ENA-

// --- Энкодеры MT6816 (SPI) ---
constexpr int PIN_SPI_SCK = 18;
constexpr int PIN_SPI_MISO = 19;
constexpr int PIN_SPI_MOSI = 23;
constexpr int PIN_CS_X = 26;
constexpr int PIN_CS_Y = 27;

// --- I2C ---
constexpr int PIN_I2C_SDA = 21;
constexpr int PIN_I2C_SCL = 22;

// --- PCA9685 ---
constexpr uint8_t PCA9685_ADDRESS = 0x40;
constexpr float PCA9685_FREQ_HZ = 50.0f;
constexpr uint16_t SERVO_MIN_US = 500;
constexpr uint16_t SERVO_MAX_US = 2500;
constexpr uint8_t SERVO_CH_ROLL_A = 0;
constexpr uint8_t SERVO_CH_ROLL_B = 1;

// ============================================================
// ENUM ДАТЧИКОВ ОРИЕНТАЦИИ (для SensorManager)
// ============================================================
enum class OrientationSensor : uint8_t
{
    NONE = 0,
    BMX055 = 1,
    MPU6500 = 2
};

// ============================================================
// КОНФИГ ШАГОВЫХ ДВИГАТЕЛЕЙ
// (структура восстановлена под StepperController.cpp)
// ============================================================
struct StepperAxisConfig
{
    uint8_t stepPin = 0;
    uint8_t dirPin = 0;
    bool invertDirection = false;
};

struct StepperConfig
{
    static constexpr uint8_t MAX_AXES = 2;

    uint8_t count = 0; // количество активных осей
    StepperAxisConfig axes[MAX_AXES];

    uint8_t enablePin = 0;       // общий EN
    bool enableActiveLow = true; // B1206: ENA- активен низким

    float stepsPerRev = 200.0f; // NEMA23, 1.8°/шаг
    float microstep = 1.0f;
};
// ============================================================
// КОНФИГ I2C (для SystemInit.cpp)
// ============================================================
struct I2CConfig
{
    uint8_t sda = PIN_I2C_SDA;
    uint8_t scl = PIN_I2C_SCL;
    uint32_t frequencyHz = 400000; // 400 кГц
};

// ============================================================
// КОНФИГ СЕРВОПРИВОДОВ (для SystemInit.cpp)
// ============================================================
struct ServoConfig
{
    uint8_t pca9685Address = PCA9685_ADDRESS;
    float frequencyHz = PCA9685_FREQ_HZ;
    uint16_t minUs = SERVO_MIN_US;
    uint16_t maxUs = SERVO_MAX_US;
    uint8_t count = 8; // 8x TD-7120MG из архитектуры
};
// ============================================================
// ОБЩИЙ КОНФИГ ПРОЕКТА
// (используется в SystemInit.cpp, SensorManager.cpp)
// ============================================================
struct ProjectConfig
{
    OrientationSensor orientationSensor = OrientationSensor::BMX055;
    StepperConfig stepper;
    control::JoystickConfig joystick;
    I2CConfig i2c;       // <-- ДОБАВЛЕНО
    ServoConfig servo;   // <-- ДОБАВЛЕНО
};

// Функция-фабрика (реализация в ProjectConfig.cpp)
ProjectConfig GetProjectConfig();

// ============================================================
// ЗАПОЛНЕННЫЙ КОНФИГ ДЖОЙСТИКА
// ============================================================
inline control::JoystickConfig getJoystickConfig()
{
    control::JoystickConfig cfg;
    cfg.pinX1 = PIN_JOY_X1;
    cfg.pinY1 = PIN_JOY_Y1;
    cfg.pinX2 = PIN_JOY_X2;
    cfg.pinY2 = PIN_JOY_Y2;
    cfg.pinK1 = PIN_JOY_K1;
    cfg.pinK2 = PIN_JOY_K2;
    cfg.pinSma = PIN_JOY_SMA;
    cfg.rawMin = 0;
    cfg.rawCenter = 2048;
    cfg.rawMax = 4095;
    cfg.oversampling = 8;
    cfg.emaAlpha = 0.35f;
    cfg.deadzone = 0.08f;
    cfg.expo = 2.2f;
    cfg.maxYawRate = 60.0f;
    cfg.maxPitchRate = 45.0f;
    cfg.maxRollRate = 90.0f;
    cfg.maxBetaRate = 60.0f;
    cfg.fineModeFactor = 0.25f;
    cfg.debounceMs = 20;
    cfg.longPressMs = 600;
    return cfg;
}