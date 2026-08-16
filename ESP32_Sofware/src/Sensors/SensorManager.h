#pragma once
#include <Wire.h>
#include "Sensors/MPU6500/MPU6500_Handler.h"
#include "Sensors/MPU6500/MPU6500_Orientation_Handler.h"
#include "Communication/DataPacket.h"

// ★ ИСПРАВЛЕНО: используем тот же тип, что в DataBlock.h
extern LocalDataBlock l_mpuData; 

class SensorManager {
private:
    TwoWire _wire;
    MPU6500_Handler _imuHandler;
    Orientation_Handler _orientationHandler;

public:
    SensorManager() : _wire(0), _imuHandler(_wire, 0x68), _orientationHandler(100.0f) {}

    bool begin() {
        if (!_imuHandler.begin()) {
            Serial.println("[FATAL] MPU6500 init failed!");
            return false;
        }
        
        Serial.println("[...] Calibrating MPU6500...");
        _imuHandler.calibrate(1000);
        _orientationHandler.begin();
        
        Serial.println("[OK] Sensor manager initialized.");
        return true;
    }

    bool readData(DataPacket& packet) {
        if (_imuHandler.readData()) {
            _orientationHandler.updateOrientation();
            
            // ★ ИСПРАВЛЕНО: используем поля LocalDataBlock
            packet.temperature = l_mpuData.temperature;
            packet.accelX = l_mpuData.accelX;
            packet.accelY = l_mpuData.accelY;
            packet.accelZ = l_mpuData.accelZ;
            packet.quatW = l_mpuData.quatW;
            packet.quatX = l_mpuData.quatX;
            packet.quatY = l_mpuData.quatY;
            packet.quatZ = l_mpuData.quatZ;
            packet.roll = l_mpuData.roll;
            packet.pitch = l_mpuData.pitch;
            packet.yaw = l_mpuData.yaw;
            
            return true;
        }
        return false;
    }
};