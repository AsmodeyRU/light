#pragma once

#include <cstdint>
#include <cstddef>

// POSIX-заголовки обязательны именно здесь, потому что мы объявляем поле типа sockaddr_in
#include <sys/socket.h>
#include <netinet/in.h>   // определение struct sockaddr_in
#include <arpa/inet.h>     // inet_pton
#include <unistd.h>        // close

#include "base.h"

class UdpPosixTransport : public Transport {
public:
    UdpPosixTransport() = default;
    explicit UdpPosixTransport(const char* ip, uint16_t port);
    ~UdpPosixTransport() override;

    bool init() override;
    bool send(const unsigned char* data, std::size_t len) override;

private:
    struct sockaddr_in _addr{};

    int _fd = -1;
    const char* _ip = nullptr;
    uint16_t _port = 0;
};
