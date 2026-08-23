cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

idf_build_set_property(COMPILE_DEFINITIONS PLATFORM_ESP32=1 APPEND)
idf_build_set_property(COMPILE_OPTIONS "-D_GNU_SOURCE" APPEND)
idf_build_set_property(MINIMAL_BUILD ON)

project(light_sensor_esp32)
