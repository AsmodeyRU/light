#include "sensors/stub.h"

Stub::Stub(SensorScenario scenario)
    : _scenario(scenario) {}

bool Stub::init(uint8_t /*address*/) {
    return true;
}

int Stub::readLux() {
    switch (_scenario) {
        case SensorScenario::Morning:  return 450;
        case SensorScenario::Evening:  return 220;
        case SensorScenario::Night:    return 50;
        default:                        return 200;
    }
}
