#include <cstddef>
#include "transports/null.h"

bool NullTransport::init() {
    // Для заглушки инициализация всегда успешна.
    // Если хочешь видеть логи при старте (даже в тесте) — раскомментируй:
    // #ifdef PLATFORM_TEST
    //     // Используй printf или свой логгер, если нет iostream
    //     printf("[NULL] Transport initialized (test mode)\n");
    // #endif
    return true;
}

bool NullTransport::send(const unsigned char* data, std::size_t len) {
    // В тестовом режиме мы просто «проглатываем» данные.
    // Но для отладки ИИ‑логики полезно видеть, что пакеты вообще формируются.

    // Вариант 1: тихий режим (ничего не делает)
    // return true;

    // Вариант 2: отладочный вывод (чтобы видеть поток данных в консоли)
    #ifdef DEBUG_NULL_SEND
        // Можно вывести длину или первые байты, если нужно
        // printf("[NULL] send(%zu bytes)\n", len);
    #endif

    return true;
}
