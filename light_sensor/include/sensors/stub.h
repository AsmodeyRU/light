#pragma once
#include "base.h"
#include "scenario.h"

class Stub : public LightSensor {
public:
    explicit Stub(SensorScenario scenario);

    bool init(uint8_t /*address*/) override;
    int readLux() override;

private:
    SensorScenario _scenario;
};
