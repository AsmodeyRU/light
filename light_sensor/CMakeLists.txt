cmake_minimum_required(VERSION 3.16)
project(light_sensor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Опции выбора платформы (взаимоисключающие) ---
option(PLATFORM_LINUX  "Linux/Orange Pi/OpenWrt (UDP POSIX)" OFF)
option(PLATFORM_TEST   "Тестовый режим (NullTransport)" ON)
option(PLATFORM_ESP32  "ESP32 (WiFi)" OFF)

# Проверка: должен быть выбран ровно один
set(PLATFORM_COUNT 0)
if(PLATFORM_LINUX)
    math(EXPR PLATFORM_COUNT "${PLATFORM_COUNT} + 1")
endif()
if(PLATFORM_TEST)
    math(EXPR PLATFORM_COUNT "${PLATFORM_COUNT} + 1")
endif()
if(PLATFORM_ESP32)
    math(EXPR PLATFORM_COUNT "${PLATFORM_COUNT} + 1")
endif()

if(${PLATFORM_COUNT} GREATER 1)
    message(FATAL_ERROR "Выберите ТОЛЬКО ОДНУ платформу: PLATFORM_LINUX / PLATFORM_TEST / PLATFORM_ESP32")
endif()
if(${PLATFORM_COUNT} EQUAL 0)
    message(FATAL_ERROR "Нужно выбрать одну платформу: -DPLATFORM_LINUX=ON ИЛИ -DPLATFORM_TEST=ON ИЛИ -DPLATFORM_ESP32=ON")
endif()

# Привязываем опции к макросам для config.hpp
if(PLATFORM_LINUX)
    add_compile_definitions(PLATFORM_LINUX=1)
    message(STATUS "Сборка: Linux/Orange Pi (UDP POSIX)")
elseif(PLATFORM_TEST)
    add_compile_definitions(PLATFORM_TEST=1)
    message(STATUS "Сборка: Тест (NullTransport)")
else() # PLATFORM_ESP32
    add_compile_definitions(PLATFORM_ESP32=1)
    message(STATUS "Сборка: ESP32 (WiFi)")
endif()

# Базовые исходники (общие для всех платформ)
set(BASE_SOURCES
    src/main.cpp
    src/sensors/bh1750.cpp
    src/sensors/stub.cpp
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Добавляем транспорт в зависимости от платформы
if(PLATFORM_LINUX)
    target_sources(${PROJECT_NAME} PRIVATE src/transports/udp_posix.cpp)
elseif(PLATFORM_TEST)
    target_sources(${PROJECT_NAME} PRIVATE src/transports/null.cpp)
else() # PLATFORM_ESP32
    target_sources(${PROJECT_NAME} PRIVATE src/transports/wifi_esp32.cpp)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(${PROJECT_NAME} PRIVATE -O2 -Wall)
endif()
