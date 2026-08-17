#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>

struct StatsData {
    uint64_t packets_received = 0;
    uint64_t bytes_received   = 0;
    uint64_t packets_sent     = 0;
    uint64_t bytes_sent       = 0;
    uint64_t errors           = 0;
};

class StatsCollector {
public:
    StatsData get_snapshot() const;

    void record(const char* ip, uint16_t port, size_t len);

    void increment_packets_received(uint64_t count = 1, uint64_t bytes = 0);
    void increment_packets_sent(uint64_t count = 1, uint64_t bytes = 0);
    void increment_errors(uint64_t count = 1);

    uint64_t packets_received() const { return packets_received_.load(std::memory_order_relaxed); }
    uint64_t bytes_received()   const { return bytes_received_.load(std::memory_order_relaxed); }
    uint64_t errors()           const { return errors_.load(std::memory_order_relaxed); }

private:
    std::atomic<uint64_t> packets_received_{0};
    std::atomic<uint64_t> bytes_received_{0};
    std::atomic<uint64_t> packets_sent_{0};
    std::atomic<uint64_t> bytes_sent_{0};
    std::atomic<uint64_t> errors_{0};
};

