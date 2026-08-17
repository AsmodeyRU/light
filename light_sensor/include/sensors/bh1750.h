#pragma once
#include "base.h"

class Bh1750 : public LightSensor {
public:
    bool init(uint8_t address) override;
    int readLux() override;
};
