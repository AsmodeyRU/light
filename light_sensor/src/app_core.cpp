#include <cstdint>

#include "sensors/base.h"
#include "sensors/stub.h"

#include "transports/transport_wrapper.hpp"
#include "config.hpp" 
#include "platform_delay.hpp"

#include "app_core.h"

int compute_dim_value(int lux) {
    if (lux < 200) return 100;
    if (lux < 400) return 60;
    return 30;
}

template <typename T>
void run_application(T& transport) {
    Stub stub(SensorScenario::Evening);
    stub.init(0x23);

    // Фиксированный буфер на стеке: ровно 2 байта
    uint8_t packet[2];

    int lux {0};
    int dim {0};

    while (true) {
        lux = stub.readLux();
        dim = compute_dim_value(lux);

        // Заполняем пакет строго по формату light_control: [ID][level]
        packet[0] = DEVICE_ID;
        packet[1] = static_cast<uint8_t>(dim);  // 0–100;

        transport.send(packet, sizeof(packet));

        platform_delay_ms(1000);
    }
}

// ============================================================
// ЯВНАЯ ИНСТАНЦИАЦИЯ (ТОЛЬКО ЗДЕСЬ, В КОНЦЕ .CPP)
// ============================================================
#if defined(PLATFORM_LINUX) || defined(TRANSPORT_HAS_IP_AND_PORT)
    #include "transports/udp_posix.h"

    template void run_application<TransportWrapper<UdpPosixTransport>>(
        TransportWrapper<UdpPosixTransport>&);

#elif defined(PLATFORM_ESP32)
    #include "transports/wifi_esp32.h"   

    template void run_application<TransportWrapper<WifiEsp32Transport >>(
        TransportWrapper<WifiEsp32Transport >&);

#else
    #error "Не определена целевая платформа: PLATFORM_LINUX или PLATFORM_ESP32"
#endif
