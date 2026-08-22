# esp32/main/CMakeLists.txt
idf_component_register(
    SRCS
        "../../src/main_esp32.cpp"
        "../../src/sensors/bh1750.cpp"
        "../../src/sensors/stub.cpp"
        "../../src/transports/wifi_esp32.cpp"
        "../../src/app_core.cpp"
    PRIV_REQUIRES nvs_flash esp_event esp_netif esp_wifi
    INCLUDE_DIRS
        "."
        "../../include"
        "../../include/sensors"
        "../../include/transports"
)
