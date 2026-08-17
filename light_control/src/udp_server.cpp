#include <sys/socket.h>
#include <sys/time.h>        
#include <netinet/in.h>      
#include <arpa/inet.h>       
#include <unistd.h>          
#include <cstring>
#include <cerrno>            
#include <cstdlib>           
#include <cstdint>           
#include <poll.h>

#include "logger.h"
#include "stats_collector.h"
#include "udp_server.h"
#include "protocol_parser.h"

UdpServer::UdpServer(StatsCollector& stats, Logger& logger)
    : stats_(stats), logger_(logger) {}

UdpServer::~UdpServer() {
    close_socket();
}

bool UdpServer::bind_to(const char* addr, uint16_t port) {
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) {
        logger_.error("socket() failed: %m");
        return false;
    }

    // SO_REUSEADDR — полезно, но не обязательно для UDP
    int reuse = 1;
    if (setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        // Не считаем это ошибкой: на некоторых ядрах может не поддерживаться
        logger_.debug("SO_REUSEADDR not supported (might be ignored on some kernels)");
    }

    struct sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(port);

    if (addr && std::strcmp(addr, "0.0.0.0") == 0) {
        local.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (!inet_pton(AF_INET, addr, &local.sin_addr)) {
            logger_.error("invalid IP address: %s", addr);
            close_socket();
            return false;
        }
    }

    if (::bind(sock_, reinterpret_cast<struct sockaddr*>(&local), sizeof(local)) < 0) {
        logger_.error("bind() failed: %m");
        close_socket();
        return false;
    }

    logger_.debug("UDP server bound to %s:%u", addr, port);
    return true;
}

void UdpServer::run(std::atomic<bool>& stop_flag) {
    char buf[256];
    char ip[INET_ADDRSTRLEN] = {0};
    uint16_t port = 0;

    struct pollfd fds[1] = {
        { .fd = sock_, .events = POLLIN }
    };

    while (!stop_flag.load(std::memory_order_acquire)) {
        // poll с таймаутом 200 мс: поток не зависнет надолго
        int ret = ::poll(fds, 1, 200);

        if (ret < 0) {
            if (errno == EINTR) {
                // Прерван сигналом — просто пробудились, проверим флаг на следующей итерации
                continue;
            }
            logger_.debug("poll error: %m (errno={})", errno);
            continue;
        }

        // Если данные готовы — читаем
        if (fds[0].revents & POLLIN) {
            ssize_t n = recv_packet(buf, sizeof(buf), ip, sizeof(ip), port);
            if (n > 0) {
                stats_.record(ip, port, static_cast<size_t>(n));
                
                // Дампим только для отладки (в проде лучше отключить DEBUG)
                logger_.debug_hex("Raw packet", buf, static_cast<size_t>(n), ip, port);

                auto res = ProtocolParser::parse(buf, static_cast<size_t>(n));

                if (res.is_valid) {
                    logger_.debug("Device %d setting dimmer to: %d",
                                static_cast<int>(res.device_id),
                                static_cast<int>(res.value));
                    // Здесь будет вызов реального диммера
                    // dimmer_control.set_brightness(res.value);
                } else {
                    stats_.increment_errors(1);
                    logger_.warn("Protocol error from %s:%u: %s",
                                ip, port, res.error_msg ? res.error_msg : "unknown");
                }
            }
        }
    }

    logger_.debug("UDP receive thread exiting gracefully");
}

ssize_t UdpServer::recv_packet(char* buf, size_t buf_len,
                               char* ip, size_t ip_len,
                               uint16_t& port) {
    struct sockaddr_in addr{};
    socklen_t addr_len = sizeof(addr);

    ssize_t n = ::recvfrom(sock_, buf, buf_len, 0,
                           reinterpret_cast<struct sockaddr*>(&addr), &addr_len);

    if (n < 0) {
        // EBADF — сокет закрыт из другого потока (это наш основной путь выхода)
        if (errno == EBADF) {
            logger_.debug("recvfrom: socket closed (EBADF), exiting loop");
            return -1;
        }

        // EAGAIN / EWOULDBLOCK — нормально для неблокирующего режима
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1;
        }

        // EINTR — это наш сигнал: сокет закрыли из stop(), надо выйти из recvfrom
        // и дать циклу run() проверить stop_flag
        if (errno == EINTR) {
            // Не считаем это ошибкой: это штатный способ разбудить поток
            return -1;
        }

        // Любые другие ошибки — логируем на debug, чтобы не засорять syslog
        logger_.debug("recvfrom() error: %m (errno={})", errno);
        return -1;
    }

    // Данные получены: заполняем выходные параметры
    port = ntohs(addr.sin_port);
    inet_ntop(AF_INET, &addr.sin_addr, ip, static_cast<socklen_t>(ip_len));

    return n;
}

void UdpServer::close_socket()
{
    if (sock_ >= 0) {
        close(sock_);
        sock_ = -1;
        logger_.debug("UDP socket closed");
    }
}

void UdpServer::stop() { 
    close_socket();
}                        
