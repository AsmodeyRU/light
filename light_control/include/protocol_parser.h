#pragma once

#include <cstdint>
#include <string_view>

class ProtocolParser {
public:
    struct Result {
        bool is_valid = false;
        uint8_t device_id = 0;
        uint8_t value = 0;
        const char* error_msg = nullptr;
    };

    // Парсит ровно 2 байта: [device_id][value]
    static Result parse(const char* data, size_t len);
};
