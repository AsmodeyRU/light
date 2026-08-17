#include "stats_collector.h"

StatsData StatsCollector::get_snapshot() const {
    return {
        .packets_received = packets_received_.load(std::memory_order_relaxed),
        .bytes_received  = bytes_received_.load(std::memory_order_relaxed),
        .packets_sent    = packets_sent_.load(std::memory_order_relaxed),
        .bytes_sent      = bytes_sent_.load(std::memory_order_relaxed),
        .errors          = errors_.load(std::memory_order_relaxed)
    };
}

void StatsCollector::record(const char*, uint16_t, size_t len) {
    packets_received_.fetch_add(1, std::memory_order_relaxed);
    if (len > 0) {
        bytes_received_.fetch_add(static_cast<uint64_t>(len), std::memory_order_relaxed);
    }
}

void StatsCollector::increment_packets_received(uint64_t count, uint64_t bytes) {
    packets_received_.fetch_add(count, std::memory_order_relaxed);
    if (bytes > 0) {
        bytes_received_.fetch_add(bytes, std::memory_order_relaxed);
    }
}

void StatsCollector::increment_packets_sent(uint64_t count, uint64_t bytes) {
    packets_sent_.fetch_add(count, std::memory_order_relaxed);
    if (bytes > 0) {
        bytes_sent_.fetch_add(bytes, std::memory_order_relaxed);
    }
}

void StatsCollector::increment_errors(uint64_t count) {
    errors_.fetch_add(count, std::memory_order_relaxed);
}
