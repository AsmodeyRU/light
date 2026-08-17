# light_sensor

A compact, memory‑safe sensor node for ambient light measurement (BH1750) with UDP reporting. Designed for embedded platforms (ESP32, Linux) under strict memory constraints: no dynamic/static allocation, no raw pointers.

## Key Features

- **Memory discipline**: strictly avoids dynamic and static memory allocation; uses stack variables and fixed‑size buffers only.
- **Modular architecture**: sensors and transports are decoupled into separate subsystems.
- **Multiple transport backends**:
  - UDP (POSIX) for Linux/OpenWrt targets.
  - Null transport for isolated testing.
  - ESP32 Wi‑Fi transport for ESP32 platforms.
- **Test‑friendly**: dedicated test build mode with stubbed hardware (no real sensor or network required).
- **Clean build configuration**: single source file list in CMake, minimal conditional compilation in core code.

## Supported Sensors

- **BH1750 (GY‑302)** – primary ambient light sensor.
- **Stub sensor** – mock implementation for CI and unit‑style tests.

## Supported Transports

- **UDP (POSIX)** – sends readings over UDP on Linux systems.
- **Null transport** – no network activity; useful for logic validation and CI.
- **ESP32 Wi‑Fi** – Wi‑Fi based transport tailored for ESP32.

## Build Requirements

- CMake ≥ 3.16
- C++17 compatible compiler (e.g., GCC 12.3.0 with musl)
- POSIX environment for Linux builds

## Building the Project

### Linux (UDP POSIX transport)

    mkdir build_linux && cd build_linux
    cmake .. -DPLATFORM_LINUX=ON -DPLATFORM_TEST=OFF -DPLATFORM_ESP32=OFF
    make -j$(nproc)
    ./light_sensor

### Test Build (No Hardware Required)
Uses stub sensor and null transport to validate logic without real hardware.

    mkdir build_test && cd build_test
    cmake .. -DPLATFORM_TEST=ON -DPLATFORM_LINUX=OFF -DPLATFORM_ESP32=OFF
    make -j$(nproc)
    ./light_sensor

### ESP32 (Wi‑Fi Transport)

    mkdir build_esp32 && cd build_esp32
    cmake .. -DPLATFORM_ESP32=ON -DPLATFORM_LINUX=OFF -DPLATFORM_TEST=OFF
    make -j$(nproc)

>Note: The ESP32 build assumes a toolchain and environment suitable for ESP32; adjust paths/flags as needed for your setup.

## Project Structure
    src/ – implementation files (.cpp).
    include/ – header files (.h, .hpp).
    CMakeLists.txt – build configuration.
    .gitignore – excludes build artifacts and IDE files.

### Example layout:

    light_sensor/
    ├── CMakeLists.txt
    ├── .gitignore
    ├── README.md
    ├── include/
    │   ├── config.hpp
    │   ├── platform_delay.hpp
    │   ├── sensors/
    │   │   ├── base.h
    │   │   ├── bh1750.h
    │   │   ├── scenario.h
    │   │   └── stub.h
    │   └── transports/
    │       ├── base.h
    │       ├── null.h
    │       ├── transport_wrapper.hpp
    │       ├── udp_posix.h
    │       └── wifi_esp32.h
    └── src/
        ├── main.cpp
        ├── sensors/
        │   ├── bh1750.cpp
        │   └── stub.cpp
        └── transports/
            ├── null.cpp
            ├── udp_posix.cpp
            └── wifi_esp32.cpp

## Configuration

Runtime behavior and platform specifics are controlled via CMake options and include/config.hpp. Avoid modifying main.cpp for platform selection; use CMake flags instead.

Platform macros injected by CMake:

- PLATFORM_LINUX – for POSIX UDP builds.
- PLATFORM_TEST – for test builds with stubs.
- PLATFORM_ESP32 – for ESP32 Wi‑Fi builds.

These macros are used in headers/sources to enable appropriate implementations without cluttering core logic.

