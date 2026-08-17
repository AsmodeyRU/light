#pragma once
#include <cstddef>

template <typename T>
class TransportWrapper {
public:
    // Пустой конструктор: просто создаёт _t.
    // Если у T нет default-ctor — эта строка сама станет ошибкой, и мы сразу это увидим.
    TransportWrapper() = default;

    explicit TransportWrapper(uint16_t port) : _t(port) {}
    explicit TransportWrapper(const char* ip, uint16_t port) : _t(ip, port) {}

    bool init() {
        return _t.init();
    }

    bool send(const unsigned char* data, std::size_t len) {
        return _t.send(data, len);
    }

private:
    T _t;
};
