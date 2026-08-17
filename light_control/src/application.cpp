#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <arpa/inet.h>

#include "application.h"

#ifdef UBUS_ENABLED
#include "ubus_exporter.h"
#endif

Application::Application()
    : logger_("light_control")
    , stats_()
    , server_(stats_, logger_)
#ifdef UBUS_ENABLED
    , ubus_(stats_, logger_)   // Инициализируем сразу
#endif
{
}

int Application::init() {
    uint16_t port = 5005;

    FILE* f = fopen("/etc/config/light_control", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "option port %hu", &port) == 1) {
                break;
            }
        }
        fclose(f);
    }

    logger_.debug("Listening port: %u", port);

    if (!server_.bind_to("0.0.0.0", port)) {
        logger_.error("Failed to bind UDP server");
        return -1;
    }

#ifdef UBUS_ENABLED
    ubus_thread_ = std::thread([this]() {
        if (!ubus_.init()) {
            logger_.debug("UBus not ready yet — will retry via uloop timer");
            // Не выходим: таймеры уже поставлены, дальше запускаем цикл
        } else {
            logger_.debug("UBus ready immediately");
        }

        ubus_.run(should_stop_);
    });
#else
    logger_.debug("UBus disabled at compile time");
#endif

    logger_.debug("Starting UDP receive thread...");

    receive_thread_ = std::thread([this]() {
        server_.run(should_stop_);
    });

    logger_.debug("Application initialized, waiting for events...");
    return 0;
}

void Application::stop() {
    logger_.debug("Stopping application components...");

    server_.stop(); // Закрываем сокет (это также прервёт recvfrom, если он завис)

    // Явно ставим флаг в true с release-семантикой
    should_stop_.store(true, std::memory_order_release);

    if (receive_thread_.joinable()) {
        receive_thread_.join();
        logger_.debug("UDP receive thread joined");
    } else {
        logger_.debug("UDP receive thread was not joinable (already finished?)");
    }

#ifdef UBUS_ENABLED
    ubus_.stop();
    if (ubus_thread_.joinable()) {
        ubus_thread_.join(); 
        logger_.debug("UBus receive thread joined");
    } else {
        logger_.debug("UBus receive thread was not joinable (already finished?)");
    }
#endif

    logger_.debug("Application stopped");
}