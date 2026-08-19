#pragma once

class NullTransport 
{
public:
    NullTransport() = default;
    ~NullTransport() = default;

    bool init();
    bool send(const unsigned char* data, std::size_t len);
};
