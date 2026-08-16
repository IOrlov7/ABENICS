#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <freertos/semphr.h>

class I2CBus {
public:
    static constexpr uint8_t DEFAULT_SDA = 21;
    static constexpr uint8_t DEFAULT_SCL = 22;

    static I2CBus& instance() {
        static I2CBus bus;
        return bus;
    }

    bool begin(
        uint8_t sda = DEFAULT_SDA,
        uint8_t scl = DEFAULT_SCL,
        uint32_t frequencyHz = 400000
    );

    bool isDevicePresent(uint8_t address);
    int scan();

    bool readRegisters(
        uint8_t address,
        uint8_t reg,
        uint8_t* data,
        size_t length
    );

    bool writeRegister(
        uint8_t address,
        uint8_t reg,
        uint8_t value
    );

    bool writeRegisters(
        uint8_t address,
        uint8_t reg,
        const uint8_t* data,
        size_t length
    );

    void lock() {
        if (_mutex != nullptr) {
            xSemaphoreTake(_mutex, portMAX_DELAY);
        }
    }

    void unlock() {
        if (_mutex != nullptr) {
            xSemaphoreGive(_mutex);
        }
    }

    class ScopedLock {
    public:
        explicit ScopedLock(I2CBus& bus) : _bus(bus) {
            _bus.lock();
        }

        ~ScopedLock() {
            _bus.unlock();
        }

        ScopedLock(const ScopedLock&) = delete;
        ScopedLock& operator=(const ScopedLock&) = delete;

    private:
        I2CBus& _bus;
    };

private:
    I2CBus();

    SemaphoreHandle_t _mutex;
    TwoWire* _wire;
    bool _ready;
};