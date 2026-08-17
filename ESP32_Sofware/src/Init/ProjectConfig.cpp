#include "Init/ProjectConfig.h"

ProjectConfig GetProjectConfig()
{
    ProjectConfig cfg;

    // --- Датчик ориентации ---
    cfg.orientationSensor = OrientationSensor::BMX055; // или MPU6500

    // --- I2C ---
    cfg.i2c.sda = PIN_I2C_SDA;
    cfg.i2c.scl = PIN_I2C_SCL;
    cfg.i2c.frequencyHz = 400000; // 400 кГц

    // --- Шаговые двигатели (2 оси, архитектура) ---
    cfg.stepper.count = 2;
    
    // Ось X
    cfg.stepper.axes[0].stepPin = PIN_STEP_X;
    cfg.stepper.axes[0].dirPin  = PIN_DIR_X;
    cfg.stepper.axes[0].invertDirection = false;

    // Ось Y
    cfg.stepper.axes[1].stepPin = PIN_STEP_Y;
    cfg.stepper.axes[1].dirPin  = PIN_DIR_Y;
    cfg.stepper.axes[1].invertDirection = false;

    cfg.stepper.enablePin = PIN_EN_ALL;
    cfg.stepper.enableActiveLow = true;
    cfg.stepper.stepsPerRev = 200.0f;
    cfg.stepper.microstep = 1.0f;

    // --- Сервоприводы (8x TD-7120MG) ---
    cfg.servo.pca9685Address = PCA9685_ADDRESS;
    cfg.servo.frequencyHz = PCA9685_FREQ_HZ;
    cfg.servo.minUs = SERVO_MIN_US;
    cfg.servo.maxUs = SERVO_MAX_US;
    cfg.servo.count = 8;

    // --- Джойстик ---
    cfg.joystick = getJoystickConfig();

    // ★ --- Сеть (NetworkConfig) ---
    cfg.network.telemetryPort       = UDP_TELEMETRY_PORT; // 8888
    cfg.network.commandPort         = UDP_COMMAND_PORT;   // 8889
    cfg.network.telemetryPeriodMs   = 20;                  // 50 Гц
    cfg.network.wifiConnectTimeoutMs = 5000;                // 5 сек
    cfg.network.useCaptivePortal    = true;

    // ★ --- Управление (ControlConfig) ---
    cfg.control.loopPeriodMs       = 20;   // 50 Гц controlTask
    cfg.control.sensorPeriodMs     = 10;   // 100 Гц sensorTask

    cfg.control.controlTaskCore    = 1;
    cfg.control.controlTaskPriority = 3;

    cfg.control.sensorTaskCore     = 1;
    cfg.control.sensorTaskPriority = 2;

    cfg.control.networkTaskCore    = 0;
    cfg.control.networkTaskPriority = 2;

    cfg.control.wifiTaskCore       = 0;
    cfg.control.wifiTaskPriority   = 1;

    return cfg;
}