#include "protocol_parser.h"

ProtocolParser::Result ProtocolParser::parse(const char* data, size_t len) {
    Result res;

    if (!data || len != 2) {
        res.error_msg = "Invalid length";
        return res;
    }

    uint8_t device_id = static_cast<uint8_t>(data[0]);
    uint8_t value     = static_cast<uint8_t>(data[1]);

    if (value > 100) {
        res.error_msg = "Value out of range (max 100)";
        return res;
    }

    res.is_valid = true;
    res.device_id = device_id;
    res.value = value;
    return res;
}
