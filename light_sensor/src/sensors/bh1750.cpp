#include "sensors/bh1750.h"

bool Bh1750::init(uint8_t address) {
    // TODO: инициализация I2C, проверка устройства по адресу, настройка режима
    return true;  // заглушка для сборки
}

int Bh1750::readLux() {
    // TODO: реальное чтение данных с BH1750
    return 250;  // заглушка
}
