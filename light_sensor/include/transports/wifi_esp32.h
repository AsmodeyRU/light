#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>

// ESP-IDF заголовки: подключаем здесь, чтобы тип sockaddr_in был полным
#include "lwip/sockets.h"

class WifiEsp32Transport 
{
    public:
        WifiEsp32Transport() = default;
        explicit WifiEsp32Transport(const char* ssid, const char* pass, const char* ip, uint16_t port);

        ~WifiEsp32Transport();

        bool init();
        bool is_initialized() const { return _initialized.load(std::memory_order_acquire); }
        void stop();
        bool send(const unsigned char* data, std::size_t len);
        
        uint32_t attempt_number() const { return _attempt.load(std::memory_order_acquire); }
        void clear_attempt_count() { _attempt.store(0, std::memory_order_release); }
        void increment_attempt_number() { _attempt++; }

    private:
        static void wifi_connect_task(void* arg);
    
    private:
        const char* _ssid {nullptr};
        const char* _pass {nullptr};
        const char* _ip {nullptr};
        uint16_t _port {};

        int _sock {-1};

        struct sockaddr_in _addr {};

        std::atomic<bool> _initialized {false};
        std::atomic<bool> _stop_requested {false};
        std::atomic<uint32_t> _attempt{0};

        TaskHandle_t _connect_task_handle{nullptr};
};

