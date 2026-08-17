#include "Init/SystemInit.h"

#include "HAL/I2CBus.h"
#include "Sensors/SensorManager.h"
#include "Motors/TD7120MG/ServoController.h"
#include "Motors/Nema23/StepperController.h"
#include "Communication/WiFiProvisioning.h"
#include "Communication/NetworkManager.h"
#include "Communication/TelemetryPacket.h"

// --- Глобальные объекты ---
static ProjectConfig g_config;

ServoController g_servoCtrl;
StepperController g_stepperCtrl;
static WiFiProvisioning g_wifi;
NetworkManager g_network; // Не static, так как возвращается через sys::network()

static bool g_systemReady = false;

// --- Глобальные переменные состояния (определения) ---
volatile CommandId g_currentCommand = CommandId::CMD_IDLE;
volatile uint8_t g_statusFlags = 0;

// --- Внешние задачи ---
extern void controlTask(void* parameter);

// --- Внутренние задачи ---
static void wifiTask(void* parameter) {
    for (;;) {
        g_wifi.handle();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void sensorTask(void* parameter) {
    while (true) {
        SensorManager::instance().update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ============================================================
//  networkTask — Core 0, приоритет 2
// ============================================================
static void networkTask(void* param) {
    const TickType_t period = pdMS_TO_TICKS(20); // 50 Гц

    for (;;) {
        // 1. Приём команд от ПК
        uint8_t cmdBuf[64];
        if (g_network.receiveCommand(cmdBuf, sizeof(cmdBuf))) {
            // TODO: Парсинг команды будет в Этапе 3
            Serial.printf("[NET] Command received, first byte=0x%02X\n", cmdBuf[0]);
        }

        // 2. Отправка телеметрии
        if (g_network.isConnected()) {
            TelemetryPacket pkt;
            memset(&pkt, 0, sizeof(pkt));

            // -- IMU --
            auto& imu = SensorManager::instance();
            pkt.imu.quat_w = imu.getQuatW();
            pkt.imu.quat_x = imu.getQuatX();
            pkt.imu.quat_y = imu.getQuatY();
            pkt.imu.quat_z = imu.getQuatZ();
            
            pkt.imu.roll   = imu.getRoll();
            pkt.imu.pitch  = imu.getPitch();
            pkt.imu.yaw    = imu.getYaw();
            
            pkt.imu.accel_x = imu.getAccelX();
            pkt.imu.accel_y = imu.getAccelY();
            pkt.imu.accel_z = imu.getAccelZ();
            pkt.imu.gyro_x  = imu.getGyroX();
            pkt.imu.gyro_y  = imu.getGyroY();
            pkt.imu.gyro_z  = imu.getGyroZ();
            pkt.imu.mag_x   = imu.getMagX();
            pkt.imu.mag_y   = imu.getMagY();
            pkt.imu.mag_z   = imu.getMagZ();
            pkt.imu.temperature = imu.getTemperature();

            // -- Шаговые моторы (заглушки, пока нет API) --
            // TODO: Заменить на реальные вызовы g_stepperCtrl.getAngleX() и т.д.
            pkt.stepper_x.angle = 0.0f; 
            pkt.stepper_x.speed = 0.0f; 
            pkt.stepper_x.direction = 0; 
            pkt.stepper_x.state = 0;
            
            pkt.stepper_y.angle = 0.0f; 
            pkt.stepper_y.speed = 0.0f; 
            pkt.stepper_y.direction = 0; 
            pkt.stepper_y.state = 0;

            // -- Сервоприводы (заглушки) --
            for (int i = 0; i < 8; i++) {
                pkt.servo_angles[i] = 0;
            }

            // -- Системное состояние --
            pkt.system.current_cmd  = (uint8_t)g_currentCommand;
            pkt.system.status_flags = g_statusFlags;
            pkt.system.wifi_rssi    = (int8_t)WiFi.RSSI();
            pkt.system.uptime_ms    = millis();

            g_network.sendTelemetry(pkt);
        }

        vTaskDelay(period);
    }
}

// ============================================================
//  Системные функции (Старый API)
// ============================================================
bool System_Init() {
    g_config = GetProjectConfig();

    auto& i2c = I2CBus::instance();
    if (!i2c.begin(g_config.i2c.sda, g_config.i2c.scl, g_config.i2c.frequencyHz)) {
        Serial.println("SystemInit: I2C init FAILED");
        return false;
    }
    Serial.println("SystemInit: I2C init OK");
    i2c.scan();

    if (SensorManager::instance().begin(g_config)) {
        Serial.println("SystemInit: SensorManager init OK");
    } else {
        Serial.println("SystemInit: SensorManager init FAILED");
    }

    if (g_servoCtrl.begin(g_config.servo.pca9685Address, g_config.servo.frequencyHz, 
                          g_config.servo.minUs, g_config.servo.maxUs, g_config.servo.count)) {
        Serial.println("SystemInit: ServoController init OK");
    } else {
        Serial.println("SystemInit: ServoController init FAILED");
    }

    if (g_stepperCtrl.begin(g_config.stepper)) {
        Serial.println("SystemInit: StepperController init OK");
    } else {
        Serial.println("SystemInit: StepperController init FAILED");
    }
    g_stepperCtrl.disableAll();

    Serial.println("SystemInit: WiFi init (non-blocking)...");
    g_wifi.begin();
    g_wifi.printStatus();

    g_network.begin();
    Serial.println("SystemInit: NetworkManager init OK");

    g_systemReady = true;
    return g_systemReady;
}

void System_StartTasks() {
    if (!g_systemReady) return;

    xTaskCreatePinnedToCore(controlTask, "ControlTask", 4096, nullptr, 3, nullptr, 1);
    xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, nullptr, 2, nullptr, 1);
    xTaskCreatePinnedToCore(wifiTask, "WifiTask", 4096, nullptr, 1, nullptr, 0);
    xTaskCreatePinnedToCore(networkTask, "NetworkTask", 8192, nullptr, 2, nullptr, 0);

    Serial.println("SystemInit: All tasks started");
}

bool System_SetServoAngle(uint8_t channel, float angleDeg) {
    return g_systemReady ? g_servoCtrl.setServoAngle(channel, angleDeg) : false;
}

bool System_SetServoMicroseconds(uint8_t channel, uint16_t pulseUs) {
    return g_systemReady ? g_servoCtrl.setServoMicroseconds(channel, pulseUs) : false;
}

bool System_EnableSteppers(bool enable) {
    if (!g_systemReady) return false;
    return enable ? g_stepperCtrl.enableAll() : g_stepperCtrl.disableAll();
}

bool System_MoveStepperSteps(uint8_t axis, int32_t steps, uint32_t stepDelayUs) {
    return g_systemReady ? g_stepperCtrl.moveSteps(axis, steps, stepDelayUs) : false;
}

bool System_GetImuData(IMU_Data& out) {
    return g_systemReady ? SensorManager::instance().getImu(out) : false;
}

bool System_WiFiConnected() {
    return g_wifi.isConnected();
}

// ============================================================
//  Новый API (namespace sys)
// ============================================================
namespace sys {
    WiFiProvisioning& wifi() { return g_wifi; }
    NetworkManager& network() { return g_network; }
    StepperController& steppers() { return g_stepperCtrl; }
    ServoController& servos() { return g_servoCtrl; }
    
    bool initAll() { return System_Init(); }
    void startTasks() { System_StartTasks(); }
}