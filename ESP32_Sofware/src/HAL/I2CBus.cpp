#include "HAL/I2CBus.h"

I2CBus::I2CBus()
    : _mutex(nullptr),
      _wire(&Wire),
      _ready(false) {
}

bool I2CBus::begin(uint8_t sda, uint8_t scl, uint32_t frequencyHz) {
    if (_ready) {
        return true;
    }

    _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr) {
        return false;
    }

    _wire->begin(sda, scl);
    _wire->setClock(frequencyHz);

    // Таймаут I2C в миллисекундах.
    // Можно увеличить, если шина длинная или есть ошибки.
    _wire->setTimeout(50);

    _ready = true;
    return true;
}

int I2CBus::scan() {
    int found = 0;

    Serial.println("I2C scan started...");

    for (uint8_t address = 8; address < 120; ++address) {
        if (isDevicePresent(address)) {
            Serial.printf("Found I2C device at 0x%02X\n", address);
            ++found;
        }
    }

    if (found == 0) {
        Serial.println("No I2C devices found");
    } else {
        Serial.printf("I2C scan finished, found devices: %d\n", found);
    }

    return found;
}

bool I2CBus::isDevicePresent(uint8_t address) {
    if (!_ready) {
        return false;
    }

    ScopedLock lock(*this);

    _wire->beginTransmission(address);
    return (_wire->endTransmission() == 0);
}

bool I2CBus::readRegisters(
    uint8_t address,
    uint8_t reg,
    uint8_t* data,
    size_t length
) {
    if (!_ready || data == nullptr || length == 0) {
        return false;
    }

    ScopedLock lock(*this);

    _wire->beginTransmission(address);
    _wire->write(reg);

    if (_wire->endTransmission(false) != 0) {
        return false;
    }

    size_t received = _wire->requestFrom(address, length);

    if (received != length) {
        return false;
    }

    for (size_t i = 0; i < length; ++i) {
        data[i] = static_cast<uint8_t>(_wire->read());
    }

    return true;
}

bool I2CBus::writeRegister(
    uint8_t address,
    uint8_t reg,
    uint8_t value
) {
    return writeRegisters(address, reg, &value, 1);
}

bool I2CBus::writeRegisters(
    uint8_t address,
    uint8_t reg,
    const uint8_t* data,
    size_t length
) {
    if (!_ready || data == nullptr || length == 0) {
        return false;
    }

    ScopedLock lock(*this);

    _wire->beginTransmission(address);
    _wire->write(reg);
    _wire->write(data, length);

    return (_wire->endTransmission() == 0);
}