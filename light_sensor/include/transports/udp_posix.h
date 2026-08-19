#pragma once

#include <netinet/in.h>   // определение struct sockaddr_in

class UdpPosixTransport 
{
    public:
        UdpPosixTransport() = default;
        explicit UdpPosixTransport(const char* ip, uint16_t port);
        ~UdpPosixTransport();

        bool init();
        bool send(const unsigned char* data, std::size_t len);

    private:
        struct sockaddr_in _addr{};

        int _fd = -1;
        const char* _ip = nullptr;
        uint16_t _port = 0;
};
