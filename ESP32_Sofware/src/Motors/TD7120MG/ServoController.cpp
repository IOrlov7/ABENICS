#include "Motors/TD7120MG/ServoController.h"
#include "HAL/I2CBus.h"

static constexpr uint8_t REG_MODE1 = 0x00;
static constexpr uint8_t REG_MODE2 = 0x01;
static constexpr uint8_t REG_LED0_ON_L = 0x06;
static constexpr uint8_t REG_ALL_LED_ON_L = 0xFA;
static constexpr uint8_t REG_PRESCALE = 0xFE;

static constexpr uint8_t MODE1_AI = 0x20;       // Auto increment
static constexpr uint8_t MODE1_SLEEP = 0x10;
static constexpr uint8_t MODE1_RESTART = 0x80;

// ВАЖНО: Теперь функция begin() имеет 5 параметров!
bool ServoController::begin(
    uint8_t address,
    float frequencyHz,
    uint16_t minUs,
    uint16_t maxUs,
    uint8_t maxChannels
) {
    _address = address;
    _frequencyHz = frequencyHz;
    _minUs = minUs;
    _maxUs = maxUs;

    _maxChannels = maxChannels;

    if (_maxChannels > 16) {
        _maxChannels = 16;
    }

    auto& bus = I2CBus::instance();

    if (!bus.isDevicePresent(_address)) {
        return false;
    }

    bool ok = true;

    // Normal mode, auto increment.
    ok &= bus.writeRegister(_address, REG_MODE1, MODE1_AI);
    delay(5);

    // Totem pole output driver.
    ok &= bus.writeRegister(_address, REG_MODE2, 0x04);
    delay(5);

    // PWM frequency, usually 50 Hz for servos.
    ok &= setFrequency(_frequencyHz);

    _ready = ok;

    return _ready;
}

bool ServoController::setFrequency(float frequencyHz) {
    if (frequencyHz < 24.0f || frequencyHz > 1526.0f) {
        return false;
    }

    auto& bus = I2CBus::instance();

    float prescaleValue = 25000000.0f / (4096.0f * frequencyHz) - 1.0f;
    uint8_t prescale = static_cast<uint8_t>(prescaleValue + 0.5f);

    uint8_t oldMode = 0;

    if (!bus.readRegisters(_address, REG_MODE1, &oldMode, 1)) {
        return false;
    }

    uint8_t sleepMode = static_cast<uint8_t>(
        (oldMode & ~MODE1_RESTART) | MODE1_SLEEP
    );

    uint8_t wakeMode = static_cast<uint8_t>(
        (oldMode & ~(MODE1_SLEEP | MODE1_RESTART)) | MODE1_AI
    );

    bool ok = true;

    ok &= bus.writeRegister(_address, REG_MODE1, sleepMode);
    ok &= bus.writeRegister(_address, REG_PRESCALE, prescale);
    ok &= bus.writeRegister(_address, REG_MODE1, wakeMode);

    delay(5);

    return ok;
}

bool ServoController::setPWM(uint8_t channel, uint16_t on, uint16_t off) {
    if (!_ready || channel >= _maxChannels || channel > 15) {
        return false;
    }

    uint8_t reg = REG_LED0_ON_L + (channel * 4);

    uint8_t data[4] = {
        static_cast<uint8_t>(on & 0xFF),
        static_cast<uint8_t>(on >> 8),
        static_cast<uint8_t>(off & 0xFF),
        static_cast<uint8_t>(off >> 8)
    };

    return I2CBus::instance().writeRegisters(_address, reg, data, sizeof(data));
}

uint16_t ServoController::microsecondsToTicks(uint16_t us) const {
    if (us == 0) {
        return 0;
    }

    uint32_t periodUs = static_cast<uint32_t>(1000000.0f / _frequencyHz);

    uint32_t ticks = (static_cast<uint32_t>(us) * 4096UL) / periodUs;

    if (ticks > 4095UL) {
        ticks = 4095UL;
    }

    return static_cast<uint16_t>(ticks);
}

bool ServoController::setServoMicroseconds(uint8_t channel, uint16_t pulseUs) {
    if (!_ready || channel >= _maxChannels || channel > 15) {
        return false;
    }

    if (pulseUs == 0) {
        return setPWM(channel, 0, 0);
    }

    return setPWM(channel, 0, microsecondsToTicks(pulseUs));
}

bool ServoController::setServoAngle(uint8_t channel, float angleDeg) {
    if (!_ready || channel >= _maxChannels || channel > 15) {
        return false;
    }

    if (angleDeg < 0.0f) {
        angleDeg = 0.0f;
    }

    if (angleDeg > 180.0f) {
        angleDeg = 180.0f;
    }

    float pulseUs =
        static_cast<float>(_minUs) +
        (angleDeg / 180.0f) * static_cast<float>(_maxUs - _minUs);

    return setServoMicroseconds(channel, static_cast<uint16_t>(pulseUs + 0.5f));
}

bool ServoController::setServoOff(uint8_t channel) {
    if (!_ready || channel >= _maxChannels || channel > 15) {
        return false;
    }

    return setPWM(channel, 0, 0);
}

bool ServoController::setAllOff() {
    if (!_ready) {
        return false;
    }

    uint8_t data[4] = {0, 0, 0, 0};

    return I2CBus::instance().writeRegisters(_address, REG_ALL_LED_ON_L, data, sizeof(data));
}

bool ServoController::readMode1(uint8_t& mode1) {
    if (!_ready) {
        return false;
    }

    return I2CBus::instance().readRegisters(_address, REG_MODE1, &mode1, 1);
}