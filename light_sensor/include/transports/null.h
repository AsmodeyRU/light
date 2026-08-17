#pragma once

#include "base.h"

class NullTransport : public Transport {
public:
    NullTransport() = default;
    ~NullTransport() override = default;

    bool init() override;
    bool send(const unsigned char* data, std::size_t len) override;
};
