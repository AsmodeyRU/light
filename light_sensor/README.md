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

#### Using build script

To build the project for Linux, use the provided build script:

    bash /home/xdcsystems/Projects/light/light_sensor/build_linux.sh

#### Manual Build Instructions

If you prefer manual build:

    mkdir build_linux && cd build_linux
    cmake .. -DPLATFORM_LINUX=ON -DPLATFORM_TEST=OFF -DPLATFORM_ESP32=OFF
    make -j$(nproc)
    ./light_sensor

#### Automated Build Script Details
The build script performs the following actions:

- Cleans up existing build directory
- Creates new build directory
- Configures project with Ninja generator
- Builds the project
- Displays path to the resulting binary

### Test Build (No Hardware Required)
Uses stub sensor and null transport to validate logic without real hardware.

#### Using build script

To build the project for Linux, use the provided build script:

    bash /home/xdcsystems/Projects/light/light_sensor/build_test_linux.sh

#### Manual Build Instructions

If you prefer manual build:

    mkdir build_test && cd build_test
    cmake .. -DPLATFORM_TEST=ON -DPLATFORM_LINUX=OFF -DPLATFORM_ESP32=OFF
    make -j$(nproc)
    ./light_sensor

#### Automated Build Script Details
The build script performs the following actions:

- Cleans up existing build directory
- Creates new build directory
- Configures project with Ninja generator
- Builds the project
- Displays path to the resulting binary


### ESP32 (Wi‑Fi Transport)

#### Prerequisites
- Python 3.7+ with required packages
- Git for repository management
- CMake ≥ 3.16
- ESP32 toolchain properly configured

#### Installing ESP-IDF

1. Navigate to externals directory:

        cd externals

2. Clone ESP-IDF repository:

        git clone --recursive -b release/v6.0.2 https://github.com/espressif/esp-idf.git

        cd esp-idf

        git submodule update --init --recursive

        # install toolchain
        ./install.sh esp32    

        # set environment
        . ./export.sh

3. Verify installation:

        idf.py --version
        idf.py check-python-dependencies

#### Building Process

To build the project for ESP32, use the provided build script:

    bash ./build_esp32.sh

The script performs the following actions:

- Checks for ESP-IDF presence
- Sets up build environment
- Cleans previous build artifacts
- Configures target to ESP32
- Initiates build process
- Deployment

After successful build, you can flash the device:

    cd esp32/build

    # Flash the application
    idf.py -p <PORT> flash

    # Monitor output
    idf.py -p <PORT> monitor

Replace <PORT> with your actual ESP32 serial port (e.g., /dev/ttyUSB0).

#### Build Artifacts
After successful build, you will find:

- Bootloader: esp32/build/bootloader/bootloader.bin
- Application image: esp32/build/*.bin

#### Troubleshooting

- Build errors - check idf.py check-python-dependencies
- Missing tools - verify toolchain installation
- Path issues - ensure correct IDF_PATH setup

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

