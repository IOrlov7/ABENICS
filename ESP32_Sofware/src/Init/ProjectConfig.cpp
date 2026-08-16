#include "Init/ProjectConfig.h"

ProjectConfig GetProjectConfig() {
    ProjectConfig cfg;

    // --- Датчик ориентации ---
    cfg.orientationSensor = OrientationSensor::BMX055;

    // --- Шаговые двигатели ---
    cfg.stepper.count = 2;
    cfg.stepper.axes[0].stepPin = PIN_STEP_X;
    cfg.stepper.axes[0].dirPin  = PIN_DIR_X;
    cfg.stepper.axes[0].invertDirection = false;
    cfg.stepper.axes[1].stepPin = PIN_STEP_Y;
    cfg.stepper.axes[1].dirPin  = PIN_DIR_Y;
    cfg.stepper.axes[1].invertDirection = false;
    cfg.stepper.enablePin       = PIN_EN_ALL;
    cfg.stepper.enableActiveLow = true;
    cfg.stepper.stepsPerRev     = 200.0f;
    cfg.stepper.microstep       = 1.0f;

    // --- Джойстик ---
    cfg.joystick = getJoystickConfig();

    // --- I2C ---
    cfg.i2c.sda         = PIN_I2C_SDA;
    cfg.i2c.scl         = PIN_I2C_SCL;
    cfg.i2c.frequencyHz = 400000;

    // --- Сервоприводы ---
    cfg.servo.pca9685Address = PCA9685_ADDRESS;
    cfg.servo.frequencyHz    = PCA9685_FREQ_HZ;
    cfg.servo.minUs          = SERVO_MIN_US;
    cfg.servo.maxUs          = SERVO_MAX_US;
    cfg.servo.count          = 8;

    return cfg;
}