#include <iostream>

#include "transports/transport_wrapper.hpp"
#include "config.hpp"
#include "app_core.h"

int main() {
    std::cout << "[INIT] Light sensor node started\n";

#if defined(TRANSPORT_HAS_IP_AND_PORT)
    TransportType transport("192.168.1.1", 5005);
#else
    TransportType transport;
#endif

    if (transport.init() == false) {
        std::cerr << "[ERROR] Transport initialization failed\n";
        return 1;
    }

    run_application(transport);
}
