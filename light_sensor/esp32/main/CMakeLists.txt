# esp32/main/CMakeLists.txt
idf_component_register(
    SRCS
        "../../src/main_esp32.cpp"
        "../../src/sensors/bh1750.cpp"
        "../../src/sensors/stub.cpp"
        "../../src/transports/wifi_esp32.cpp"
        "../../src/app_core.cpp"
    INCLUDE_DIRS
        "."
        "../../include"
        "../../include/sensors"
        "../../include/transports"
)
