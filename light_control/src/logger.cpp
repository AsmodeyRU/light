#include <cstdio>
#include <cstdarg>
#include <syslog.h>
#include <sstream>
#include <iomanip>

#include "logger.h"

Logger::Logger(const char* ident) {
    openlog(ident, LOG_PID | LOG_NDELAY, LOG_LOCAL0);
}

Logger::~Logger() {
    closelog();
}

void Logger::info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsyslog(LOG_INFO, fmt, args);
    va_end(args);
}

void Logger::debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsyslog(LOG_DEBUG, fmt, args);
    va_end(args);
}

void Logger::warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsyslog(LOG_WARNING, fmt, args);
    va_end(args);
}

void Logger::error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsyslog(LOG_ERR, fmt, args);
    va_end(args);
}

void Logger::debug_hex(const char* prefix, const char* data, size_t len,
                       const char* ip, uint16_t port) {
    std::ostringstream oss;
    oss << prefix << ": len=" << len
        << ", from " << ip << ":" << port
        << ", hex=";

    oss << std::hex << std::uppercase;
    for (size_t i = 0; i < len; ++i) {
        unsigned char b = static_cast<unsigned char>(data[i]);
        oss << std::setw(2) << std::setfill('0') << static_cast<int>(b);
        if (i + 1 < len) {
            oss << ' ';
        }
    }

    syslog(LOG_DEBUG, "%s", oss.str().c_str());
}
