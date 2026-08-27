#include <array>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/tcp.h>

#include <openssl/evp.h>  // AES
#include <openssl/hmac.h> // HMAC

#include "dimmer.h"
#include "logger.h"

namespace {
    constexpr int DP_SWITCH = 20;
    constexpr int DP_MODE   = 21;
    constexpr int DP_BRIGHT = 22;

    // Простая таблица CRC32 
    int32_t crc32_table[256];
    bool crc32_initialized = false;

    void init_crc32() {
        if (crc32_initialized) return;
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t crc = i;
            for (int j = 0; j < 8; ++j)
                crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320UL : 0);
            crc32_table[i] = crc;
        }
        crc32_initialized = true;
    }

    uint32_t calc_crc32(const uint8_t* data, size_t len) {
        init_crc32();
        uint32_t crc = 0xFFFFFFFFUL;
        for (size_t i = 0; i < len; ++i)
            crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
        return crc ^ 0xFFFFFFFFUL;
    }

    bool aes_ecb_encrypt(const uint8_t* key, const uint8_t* in, size_t in_len,
                        uint8_t* out, size_t* out_len) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return false;

        if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr, key, nullptr)) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }

        // PKCS7 padding — OpenSSL включает по умолчанию, но укажем явно
        EVP_CIPHER_CTX_set_padding(ctx, 1);

        int len = 0;
        if (!EVP_EncryptUpdate(ctx, out, &len, in, static_cast<int>(in_len))) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        int final_len = 0;
        if (!EVP_EncryptFinal_ex(ctx, out + len, &final_len)) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }

        *out_len = static_cast<size_t>(len + final_len);
        EVP_CIPHER_CTX_free(ctx);
        return true;
    }
}

Dimmer::Dimmer(Logger& logger)
    : logger_(logger), socket_(-1), seq_(0), has_last_(false), last_(0) {}

Dimmer::~Dimmer() {
    if (socket_ >= 0) {
        close(socket_);
    }
}

// Your LAN and firewall will need to allow UDP (6666, 6667 and 7000) and TCP (6668) traffic.

// Порт	Протокол	Назначение
// 6666	UDP	Discovery (старые устройства v3.1)
// 6667	UDP	Discovery (v3.3, зашифрованный броадкаст)
// 7000	UDP	Discovery (доп. порт сканирования)
// 6668	TCP	Команды управления (v3.3+)
bool Dimmer::init(const std::string& tuya_ip, const std::string& local_key) {
    if (socket_ >= 0) {
        close(socket_);
        socket_ = -1;
    }

    memset(&tuya_addr_, 0, sizeof(tuya_addr_));
    seq_ = 0;

    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        logger_.error("Dimmer: Failed to create TCP socket");
        return false;
    }

    tuya_addr_.sin_family = AF_INET;
    tuya_addr_.sin_port = htons(port_);
    if (inet_pton(AF_INET, tuya_ip.c_str(), &tuya_addr_.sin_addr) <= 0) {
        logger_.error("Dimmer: Invalid IP address");
        close(socket_);
        socket_ = -1;
        return false;
    }

    // TCP_NODELAY — отключаем алгоритм Нагла, чтобы пакеты уходили сразу
    int flag = 1;
    setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    // Таймауты на приём/отправку
    struct timeval tv{};
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    // Подключаемся (TCP)
    if (connect(socket_, reinterpret_cast<struct sockaddr*>(&tuya_addr_),
                sizeof(tuya_addr_)) < 0) {
        logger_.error("Dimmer: TCP connect failed: %s", strerror(errno));
        close(socket_);
        socket_ = -1;
        return false;
    }

    if (local_key.size() != 16) {
        logger_.error("Dimmer: local_key must be exactly 16 bytes, got %zu. "
                      "Check UCI value and shell escaping.", local_key.size());
        close(socket_);
        socket_ = -1;
        return false;
    }

    memcpy(local_key_bin_, local_key.data(), 16);

    logger_.info("Dimmer: TCP connected to %s port %u", tuya_ip.c_str(), port_);
    return true;
}

// пакет v3.3
// Из протокола: 
// 000055aa SSSSSSSS MMMMMMMM LLLLLLLL DD..DD CC..CC 0000aa55
// Поле	Размер	Описание
// Prefix	4	00 00 55 AA
// Sequence	4	sequence number (big-endian)
// Command	4	00 00 00 07 (SET)
// Length	4	длина от payload до footer включительно
// Payload	переменная	version header (15 байт, open text) + AES-128-ECB шифротекст
// CRC32	4	CRC32 от byte 4 до конца payload
// Footer	4	00 00 AA 55
//
// Version header для CONTROL-команд — это "3.3" (3 байта) + 12 нулей = 15 байт, пишется до зашифрованных данных в открытом виде. 
//
// Length = 15 + enc_len + 4 (CRC) + 4 (footer).
//
void Dimmer::build_tuya_packet(uint8_t* out_buf, size_t& out_len,
                                const uint8_t* plain_payload, size_t plain_len,
                                uint32_t seq_val) {
    // 1. AES-128-ECB encrypt (с PKCS7 padding)
    uint8_t enc_payload[512] = {};
    size_t enc_len = 0;

    if (!aes_ecb_encrypt(local_key_bin_, plain_payload, plain_len,
                         enc_payload, &enc_len)) {
        logger_.error("Dimmer: AES-ECB encryption failed");
        out_len = 0;
        return;
    }

    logger_.debug("Tuya: enc_len=%zu (AES-ECB)", enc_len);

    // 2. Version header: "3.3" + 12 nulls = 15 bytes (CLEAR TEXT)
    //    Для CONTROL (cmd 7) — обязателен
    static const uint8_t version_header[15] = {
        '3', '.', '3',
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    // 3. Length = version_header(15) + enc_len + CRC(4) + footer(4)
    uint32_t total_len = static_cast<uint32_t>(15 + enc_len + 4 + 4);

    // 4. Build packet
    size_t offset = 0;

    // Prefix: 00 00 55 AA
    out_buf[offset++] = 0x00;
    out_buf[offset++] = 0x00;
    out_buf[offset++] = 0x55;
    out_buf[offset++] = 0xAA;

    // Sequence number (big-endian)
    uint32_t seq_be = htonl(seq_val);
    memcpy(out_buf + offset, &seq_be, 4);
    offset += 4;

    // Command: 00 00 00 07 (SET)
    out_buf[offset++] = 0x00;
    out_buf[offset++] = 0x00;
    out_buf[offset++] = 0x00;
    out_buf[offset++] = 0x07;

    // Length (big-endian)
    uint32_t len_be = htonl(total_len);
    memcpy(out_buf + offset, &len_be, 4);
    offset += 4;

    // Version header (15 bytes, clear text)
    memcpy(out_buf + offset, version_header, 15);
    offset += 15;

    // Encrypted payload
    memcpy(out_buf + offset, enc_payload, enc_len);
    offset += enc_len;

    // CRC32: from byte 4 to here (без prefix, без CRC/footer)
    uint32_t crc = calc_crc32(out_buf, offset);
    uint32_t crc_be = htonl(crc);
    memcpy(out_buf + offset, &crc_be, 4);
    offset += 4;

    // Footer: 00 00 AA 55
    out_buf[offset++] = 0x00;
    out_buf[offset++] = 0x00;
    out_buf[offset++] = 0xAA;
    out_buf[offset++] = 0x55;

    out_len = offset;
}

bool Dimmer::reconnect() {
    if (socket_ >= 0) {
        close(socket_);
        socket_ = -1;
    }

    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        logger_.error("Dimmer: reconnect: socket() failed: %s", strerror(errno));
        return false;
    }

    int flag = 1;
    setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    struct timeval tv{};
    tv.tv_sec = 5;
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(socket_, reinterpret_cast<struct sockaddr*>(&tuya_addr_),
                sizeof(tuya_addr_)) < 0) {
        logger_.error("Dimmer: reconnect: connect() failed: %s", strerror(errno));
        close(socket_);
        socket_ = -1;
        return false;
    }

    logger_.info("Dimmer: reconnected to %s port %u",
                 inet_ntoa(tuya_addr_.sin_addr), port_);
    return true;
}

bool Dimmer::send_packet(const uint8_t* payload, size_t payload_len) {
    if (socket_ < 0) {
        logger_.warn("Dimmer: socket closed, attempting reconnect...");
        if (!reconnect()) {
            return false;
        }
    }

    seq_++;
    uint32_t current_seq = static_cast<uint32_t>(seq_);

    uint8_t packet_buf[1024] = {};
    size_t packet_len = 0;
    build_tuya_packet(packet_buf, packet_len, payload, payload_len, current_seq);

    if (packet_len == 0 || packet_len > sizeof(packet_buf)) {
        logger_.error("Dimmer::send_packet: invalid packet length=%zu", packet_len);
        return false;
    }

    logger_.debug("Tuya: packet[0..23]:");
    for (size_t i = 0; i < 24 && i < packet_len; ++i) {
        logger_.debug("%02x", packet_buf[i]);
        if (i > 0 && (i + 1) % 8 == 0) logger_.debug("");
    }
    logger_.debug("");

    // Отправка через TCP
    ssize_t sent = send(socket_, packet_buf, packet_len, 0);
    if (sent < 0) {
        int err = errno;
        logger_.error("Dimmer: send() failed, errno=%d (%s), reconnecting...", err, strerror(err));
        if (reconnect()) {
            sent = send(socket_, packet_buf, packet_len, 0);
        }
    }

    if (sent < 0 || static_cast<size_t>(sent) != packet_len) {
        logger_.error("Dimmer: send incomplete or failed, sent=%ld", static_cast<long>(sent));
        return false;
    }

    logger_.debug("Tuya: packet sent, len=%zu", static_cast<size_t>(sent));

    // Приём ответа (TCP)
    uint8_t recv_buf[512] = {};
    ssize_t n = recv(socket_, recv_buf, sizeof(recv_buf), 0);

    if (n > 0) {
        logger_.debug("Tuya: received %ld bytes", static_cast<long>(n));
        // Здесь можно добавить разбор ответа: cmd=8, retcode=0 и т.д.
    } else if (n == 0) {
        // TCP: n==0 — соединение закрыто устройством
        logger_.warn("Tuya: connection closed by device");
        close(socket_);
        socket_ = -1;
        return false;
    } else {
        int err = errno;
        if (err == EAGAIN || err == EWOULDBLOCK) {
            // Таймаут — нормально, устройство может не отвечать
            logger_.debug("Tuya: no response (timeout, errno=%d)", err);
            return true;
        } else {
            logger_.error("Tuya: recv() failed, errno=%d (%s)", err, strerror(err));
            close(socket_);
            socket_ = -1;
            return false;
        }
    }

    return true;
}

void Dimmer::set_brightness(uint8_t brightness) {
    if (brightness > 100) brightness = 100;

    if (has_last_ && last_ == brightness) {
        return;
    }

    last_ = brightness;
    has_last_ = true;
    logger_.info("dimmer try set_brightness %u", static_cast<unsigned>(brightness));

    int tuya_val = brightness * 10;

    char json_buf[256] = {};
    time_t now = time(nullptr);

    int written = snprintf(json_buf, sizeof(json_buf),
        "{\"devId\":\"%s\",\"uid\":\"%s\",\"t\":%ld,\"dps\":{\"%u\":%u}}",
        device_id_.c_str(),
        device_id_.c_str(),
        static_cast<long>(now),
        DP_BRIGHT,
        tuya_val);

    if (written < 0 || static_cast<size_t>(written) >= sizeof(json_buf)) {
        logger_.error("Tuya: payload too large or snprintf failed");
        return;
    }
    logger_.debug("Tuya: sending payload: %s", json_buf);

    send_packet(reinterpret_cast<const uint8_t*>(json_buf), strlen(json_buf));
}
