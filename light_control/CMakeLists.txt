cmake_minimum_required(VERSION 3.16)
project(light_control CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_UBUS "Enable UBus support" OFF)

# Если OPENWRT_BUILD задан снаружи (через CMAKE_OPTIONS), включаем UBus
if(OPENWRT_BUILD)
    set(ENABLE_UBUS ON CACHE BOOL "Force UBus for OpenWRT" FORCE)
endif()

if(ENABLE_UBUS)
    add_compile_definitions(UBUS_ENABLED)
endif()

add_executable(light_control)

target_sources(light_control PRIVATE
    src/main.cpp
    src/application.cpp
    src/udp_server.cpp
    src/protocol_parser.cpp
    src/logger.cpp
    src/stats_collector.cpp
    src/ubus_exporter.cpp
    src/uci_settings.cpp
    src/brightness_curve.cpp
    src/last_status.cpp
    src/scene_scheduler.cpp
    src/dimmer.cpp
)

target_include_directories(light_control PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libubox
)

# atomic нужен на MIPS для std::atomic
if(OPENWRT_BUILD)
    target_compile_definitions(light_control PRIVATE OPENWRT_BUILD)
    target_link_libraries(light_control PRIVATE atomic uci crypto z)

    # Здесь можно оставить crypto/z, ЕСЛИ ты включил их через menuconfig и PKG_BUILD_DEPENDS
    # Но лучше убрать, если планируешь использовать встроенные реализации (без OpenSSL/zlib)
    # target_link_libraries(light_control PRIVATE crypto z)
endif()

if(ENABLE_UBUS AND OPENWRT_BUILD)
    target_link_libraries(light_control PRIVATE ubox ubus)
endif()

message(STATUS "OpenWRT build: ${OPENWRT_BUILD}")
message(STATUS "UBus enabled: ${ENABLE_UBUS}")

if(OPENWRT_BUILD)
    install(TARGETS light_control DESTINATION bin)
endif()