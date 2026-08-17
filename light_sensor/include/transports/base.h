#pragma once
#include <cstddef>
#include <cstdint>

class Transport {
public:
    virtual ~Transport() = default;
    virtual bool init() = 0;
    virtual bool send(const unsigned char* data, std::size_t len) = 0;
};
