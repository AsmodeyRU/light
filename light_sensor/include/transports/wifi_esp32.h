#pragma once

#include <cstdint>
#include <cstddef>

// ESP-IDF заголовки: подключаем здесь, чтобы тип sockaddr_in был полным
#include "lwip/sockets.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>      // close

class WifiEsp32Transport 
{
    public:
        WifiEsp32Transport() = default;
        explicit WifiEsp32Transport(const char* ssid, const char* pass, const char* ip, uint16_t port)
            : _ssid(ssid), _pass(pass), _ip(ip), _port(port) {}

        ~WifiEsp32Transport();

        bool init();
        bool send(const unsigned char* data, std::size_t len);

    private:
        const char* _ssid = nullptr;
        const char* _pass = nullptr;
        const char* _ip = nullptr;
        uint16_t _port = 0;

        int _sock = -1;
        bool _initialized = false;

        struct sockaddr_in _addr{};
};

