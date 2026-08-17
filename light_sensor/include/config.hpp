#pragma once

// ------------------------------------------------------------------
// ПЛАТФОРМА ЗАДАЁТСЯ ТОЛЬКО В CMake (add_compile_definitions)
// НЕ РАСКОММЕНТИРУЙ НИ ОДИН #define ЗДЕСЬ!
// ------------------------------------------------------------------

#if defined(PLATFORM_ESP32)
    #include "transports/wifi_esp32.h"
    using TransportType = TransportWrapper<WifiEsp32Transport>;
    #define TRANSPORT_HAS_PORT

#elif defined(PLATFORM_LINUX)
    #include "transports/udp_posix.h"
    using TransportType = TransportWrapper<UdpPosixTransport>;
    #define TRANSPORT_HAS_IP_AND_PORT

#elif defined(PLATFORM_TEST)
    #include "transports/null.h"
    using TransportType = TransportWrapper<NullTransport>;
    // Для NullTransport не нужны параметры, поэтому макросы не задаём

#else
    #error "No platform defined! Use CMake: -DPLATFORM_TEST=1 or -DPLATFORM_LINUX=1 or -DPLATFORM_ESP32=1"
#endif
