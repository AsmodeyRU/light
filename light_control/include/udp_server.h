#pragma once

#include <cstdint>
#include <cstddef>
#include <atomic>
#include <sys/types.h>

class Logger;
class StatsCollector;

class UdpServer {
public:
    explicit UdpServer(StatsCollector& stats, Logger& logger);
    ~UdpServer();

    bool bind_to(const char* addr, uint16_t port);

    void run(std::atomic<bool>& stop_flag);
    void stop();

private:
    // Приём одного UDP‑пакета. Возвращает -1 при ошибке или если сервер остановлен.
    ssize_t recv_packet(char* buf, size_t buf_len,
                        char* ip, size_t ip_len,
                        uint16_t& port);
    
    void close_socket(); 

private:
    int sock_ = -1;                 // Один сокет, без дублей
    StatsCollector& stats_;
    Logger& logger_;
};
