#pragma once
#include <cstdint>
#include "scenario.h"

class LightSensor {
public:
    virtual ~LightSensor() = default;
    virtual bool init(uint8_t address) = 0;
    virtual int readLux() = 0;
};
