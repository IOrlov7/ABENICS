#include "Sensors/BMX055/BMX055_Handler.h"
#include "HAL/I2CBus.h"

static constexpr uint8_t ACC_REG_DATA = 0x02;
static constexpr uint8_t GYR_REG_DATA = 0x02;
static constexpr uint8_t MAG_REG_DATA = 0x03;

// Для выбранных диапазонов.
// Если будешь менять диапазон в initAccel/initGyro,
// обязательно меняй и эти коэффициенты.
static constexpr float ACCEL_LSB_PER_G = 1024.0f;     // ±2g
static constexpr float GYRO_LSB_PER_DPS = 65.536f;    // ±500 dps

bool BMX055_Handler::begin() {
    auto& bus = I2CBus::instance();

    // Акселерометр BMX055 может быть по адресу 0x18 или 0x19.
    _addrAccel = 0x18;
    _accelPresent = bus.isDevicePresent(_addrAccel);

    if (!_accelPresent) {
        _addrAccel = 0x19;
        _accelPresent = bus.isDevicePresent(_addrAccel);
    }

    // Гироскоп BMX055 может быть по адресу 0x68 или 0x69.
    _addrGyro = 0x68;
    _gyroPresent = bus.isDevicePresent(_addrGyro);

    if (!_gyroPresent) {
        _addrGyro = 0x69;
        _gyroPresent = bus.isDevicePresent(_addrGyro);
    }

    // Магнитометр BMX055 может быть по адресу 0x10 или 0x12.
    _addrMag = 0x10;
    _magPresent = bus.isDevicePresent(_addrMag);

    if (!_magPresent) {
        _addrMag = 0x12;
        _magPresent = bus.isDevicePresent(_addrMag);
    }

    bool ok = false;

    if (_accelPresent) {
        initAccel();
        ok = true;
    }

    if (_gyroPresent) {
        initGyro();
        ok = true;
    }

    if (_magPresent) {
        initMag();
    }

    return ok;
}

bool BMX055_Handler::read(BMX055Data& data) {
    data = BMX055Data();
    data.timestampMs = millis();

    if (_accelPresent) {
        data.accelOk = readAccel(data.accelG);
    }

    if (_gyroPresent) {
        data.gyroOk = readGyro(data.gyroDps);
    }

    if (_magPresent) {
        data.magOk = readMagRaw(data.magRaw);
    }

    return data.accelOk || data.gyroOk;
}

bool BMX055_Handler::initAccel() {
    auto& bus = I2CBus::instance();

    bool ok = true;

    // Диапазон ±2g.
    // При смене диапазона изменить ACCEL_LSB_PER_G.
    ok &= bus.writeRegister(_addrAccel, 0x0F, 0x03);

    // Нормальный режим и полоса пропускания.
    // При необходимости уточнить значение под конкретную ревизию/задачу.
    ok &= bus.writeRegister(_addrAccel, 0x10, 0x0C);

    delay(10);

    return ok;
}

bool BMX055_Handler::initGyro() {
    auto& bus = I2CBus::instance();

    bool ok = true;

    // Диапазон ±500 dps.
    // При смене диапазона изменить GYRO_LSB_PER_DPS.
    ok &= bus.writeRegister(_addrGyro, 0x0F, 0x02);

    // Нормальный режим гироскопа.
    ok &= bus.writeRegister(_addrGyro, 0x11, 0x00);

    delay(10);

    return ok;
}

bool BMX055_Handler::initMag() {
    auto& bus = I2CBus::instance();

    bool ok = true;

    // Базовое включение магнитометра.
    // Для нормального использования нужна компенсация по trim-коэффициентам.
    // Пока можно воспринимать это как experimental raw read.
    ok &= bus.writeRegister(_addrMag, 0x4B, 0x01); // Power control
    ok &= bus.writeRegister(_addrMag, 0x4C, 0x00); // Operation mode
    ok &= bus.writeRegister(_addrMag, 0x4E, 0x04); // XY repetitions
    ok &= bus.writeRegister(_addrMag, 0x51, 0x0F); // Z repetitions

    delay(10);

    return ok;
}

bool BMX055_Handler::readAccel(float accelG[3]) {
    uint8_t buf[6];

    if (!I2CBus::instance().readRegisters(_addrAccel, ACC_REG_DATA, buf, sizeof(buf))) {
        return false;
    }

    for (int axis = 0; axis < 3; ++axis) {
        uint8_t lsb = buf[axis * 2 + 0];
        uint8_t msb = buf[axis * 2 + 1];

        int16_t raw = static_cast<int16_t>((msb << 4) | (lsb >> 4));

        // 12-bit signed conversion.
        if (raw > 2047) {
            raw -= 4096;
        }

        accelG[axis] = static_cast<float>(raw) / ACCEL_LSB_PER_G;
    }

    return true;
}

bool BMX055_Handler::readGyro(float gyroDps[3]) {
    uint8_t buf[6];

    if (!I2CBus::instance().readRegisters(_addrGyro, GYR_REG_DATA, buf, sizeof(buf))) {
        return false;
    }

    for (int axis = 0; axis < 3; ++axis) {
        uint8_t lsb = buf[axis * 2 + 0];
        uint8_t msb = buf[axis * 2 + 1];

        int16_t raw = static_cast<int16_t>((msb << 8) | lsb);

        gyroDps[axis] = static_cast<float>(raw) / GYRO_LSB_PER_DPS;
    }

    return true;
}

bool BMX055_Handler::readMagRaw(int16_t magRaw[3]) {
    uint8_t buf[6];

    if (!I2CBus::instance().readRegisters(_addrMag, MAG_REG_DATA, buf, sizeof(buf))) {
        return false;
    }

    int16_t x = static_cast<int16_t>((buf[1] << 8) | buf[0]);
    int16_t y = static_cast<int16_t>((buf[3] << 8) | buf[2]);
    int16_t z = static_cast<int16_t>((buf[5] << 8) | buf[4]);

    // BMM150/BMX055 magnetometer raw alignment:
    // X/Y approximately 13-bit, Z approximately 15-bit.
    magRaw[0] = static_cast<int16_t>(x >> 3);
    magRaw[1] = static_cast<int16_t>(y >> 3);
    magRaw[2] = static_cast<int16_t>(z >> 1);

    return true;
}