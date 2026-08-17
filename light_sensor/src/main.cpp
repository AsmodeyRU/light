#include <cstdint>

#if defined(PLATFORM_LINUX) || defined(PLATFORM_TEST)
    #include <iostream>
#endif

#include "sensors/base.h"
#include "sensors/stub.h"

#include "transports/transport_wrapper.hpp"
#include "config.hpp"      // <-- тут определяется TransportType и флаги
#include "platform_delay.hpp"

int compute_dim_value(int lux) {
    if (lux < 200) return 100;
    if (lux < 400) return 60;
    return 30;
}

int main() {
#if defined(PLATFORM_LINUX) || defined(PLATFORM_TEST)
    std::cout << "[INIT] Light sensor node started\n";
#endif

    // ВАЖНО: создаём объект сразу с нужными параметрами.
    // Никаких operator= и лишних присваиваний.
#if defined(TRANSPORT_HAS_PORT)
    TransportType transport(5005);
#elif defined(TRANSPORT_HAS_IP_AND_PORT)
    TransportType transport("192.168.1.1", 5005);
#else
    // Для NullTransport (и любых других без параметров) — конструктор по умолчанию
    TransportType transport;
#endif

    if (!transport.init()) {
#if defined(PLATFORM_LINUX) || defined(PLATFORM_TEST)
        std::cerr << "[ERROR] Transport initialization failed\n";
#else
        // На ESP32 логирование уже внутри wifi_esp32.cpp (ESP_LOG*)
#endif
        return 1;
    }

    Stub stub(SensorScenario::Evening);
    stub.init(0x23);

    while (true) {
        int lux = stub.readLux();
        int dim = compute_dim_value(lux);
        uint8_t b = static_cast<uint8_t>(dim);

        // Отправляем ровно 1 байт — диммер (0–100)
        transport.send(&b, 1);

        platform_delay_ms(1000);
    }
}
