#pragma once

// ------------------------------------------------------------------
// ГЛОБАЛЬНЫЕ ДЕФОЛТЫ (единая точка правды для всех платформ)
// Эти значения используются как fallback, если платформа не переопределила их.
// ------------------------------------------------------------------
constexpr uint8_t DEVICE_ID = 1;

// ------------------------------------------------------------------
// ПЛАТФОРМА ЗАДАЁТСЯ ТОЛЬКО В CMake (add_compile_definitions)
// НЕ РАСКОММЕНТИРУЙ НИ ОДИН #define ЗДЕСЬ!
// ------------------------------------------------------------------

#if defined(PLATFORM_ESP32)
    #include "transports/wifi_esp32.h"
    using TransportType = TransportWrapper<WifiEsp32Transport>;
    #define TRANSPORT_HAS_WIFI

    // ESP‑IDF подставит CONFIG_* из sdkconfig
    static constexpr const char* kWifiSsid = CONFIG_LIGHT_SENSOR_WIFI_SSID;
    static constexpr const char* kWifiPass = CONFIG_LIGHT_SENSOR_WIFI_PASS;
    static constexpr const char* kControllerIp = CONFIG_LIGHT_SENSOR_CONTROLLER_IP;
    static constexpr uint16_t kControllerPort = CONFIG_LIGHT_SENSOR_CONTROLLER_PORT;

    constexpr const char* WIFI_SSID = kWifiSsid;
    constexpr const char* WIFI_PASS = kWifiPass;
    constexpr const char* CONTROLLER_IP = kControllerIp;
    constexpr uint16_t CONTROLLER_PORT = kControllerPort;

    constexpr unsigned SAMPLE_PERIOD_MS = 1000;
    constexpr uint8_t BH1750_I2C_ADDR = 0x23;

    constexpr int I2C_SDA_GPIO = 21;
    constexpr int I2C_SCL_GPIO = 22;
    constexpr uint32_t I2C_HZ = 100000;

#elif defined(PLATFORM_LINUX)
    #include "transports/udp_posix.h"
    using TransportType = TransportWrapper<UdpPosixTransport>;
    #define TRANSPORT_HAS_IP_AND_PORT

    static constexpr const char* kControllerIp = "192.168.1.1";
    static constexpr uint16_t kControllerPort = 5005;

    constexpr const char* CONTROLLER_IP = kControllerIp;
    constexpr uint16_t CONTROLLER_PORT = kControllerPort;

#elif defined(PLATFORM_TEST)
    #include "transports/null.h"
    using TransportType = TransportWrapper<NullTransport>;
#else
    #error "No platform defined! Use CMake: -DPLATFORM_TEST=1 or -DPLATFORM_LINUX=1 or -DPLATFORM_ESP32=1"
#endif
