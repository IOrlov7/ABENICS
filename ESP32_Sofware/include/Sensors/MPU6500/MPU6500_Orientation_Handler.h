#ifndef ORIENTATION_HANDLER_H
#define ORIENTATION_HANDLER_H

#include <Arduino.h>
#include <MadgwickAHRS.h> // Стандартный заголовок библиотеки
#include "DataBlock.h"

class Orientation_Handler {
public:
    Orientation_Handler(float freq);
    void begin();
    bool updateOrientation();

private:
    float sampleFreq;
    Madgwick filter; // Объявляем объект БЕЗ параметров
};

#endif