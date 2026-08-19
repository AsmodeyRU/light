#pragma once

#include <cstddef>
#include <utility> 

template <typename T>
class TransportWrapper {
public:
    // Пустой конструктор: просто создаёт _t.
    // Если у T нет default-ctor — эта строка сама станет ошибкой, и мы сразу это увидим.
    TransportWrapper() = default;

    template <typename... Args>
    explicit TransportWrapper(Args&&... args)
        : _t(std::forward<Args>(args)...) {}


    bool init() {
        return _t.init();
    }

    bool send(const unsigned char* data, std::size_t len) {
        return _t.send(data, len);
    }

private:
    T _t;
};
