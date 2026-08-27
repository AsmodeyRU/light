#pragma once

#include <cstdint>
#include <string>
#include <netinet/in.h>

class Logger;

class Dimmer {
public:
    explicit Dimmer(Logger& logger);
    ~Dimmer();

    // Инициализация: передаём IP и RAW-ключ (без MD5, без HEX)
    bool init(const std::string& tuya_ip, const std::string& local_key);

    void set_brightness(uint8_t brightness);

    uint8_t last_brightness() const { return last_; }
    bool has_output() const { return has_last_; }
    bool is_ready() const { return socket_ >= 0; }

private:
    void build_tuya_packet(uint8_t* out_buf, size_t& out_len, const uint8_t* plain_payload, size_t plain_len, uint32_t seq_val);
    bool send_packet(const uint8_t* payload, size_t payload_len);
    bool reconnect();

private:
    Logger& logger_;

    int socket_ = -1;
    struct sockaddr_in tuya_addr_{};
    uint16_t port_ { 6668 };

    std::string device_id_ = "80735068bcff4d083a98";
    uint8_t local_key_bin_[16];

    // Счётчик пакетов: int достаточно (переполнение наступит через годы непрерывной работы)
    int seq_ = 0;

    uint8_t last_ = 0;
    bool has_last_ = false;
};
