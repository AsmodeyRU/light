#include <unistd.h>
#include <signal.h>
#include <atomic>

#include "application.h"

int main(int argc, char **argv) {
    // Блокируем SIGTERM и SIGINT, чтобы они не прерывали потоки,
    // а ждали в sigwait в главном потоке.
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);

    if (sigprocmask(SIG_BLOCK, &mask, nullptr) != 0) {
        return 1;
    }

    Application app;

     if (app.init() != 0) {
        // Если инициализация не удалась — просто выходим,
        // stop() вызывать не нужно: потоки ещё не запущены.
        return 1;
    }

    // Главный поток ждёт прихода сигнала.
    int sig;
    if (sigwait(&mask, &sig) == 0) {
        // Получили сигнал: корректно останавливаем приложение.
        app.stop();
    } else {
        // На случай ошибки sigwait — всё равно пытаемся остановиться.
        app.stop();
    }

    return 0;
}

