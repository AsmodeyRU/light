#include <unistd.h>
#include <cerrno>
#include <cstring>

#include <thread>
#include <chrono>

#include <syslog.h>

#include "logger.h"
#include "ubus_exporter.h"

#ifdef UBUS_ENABLED

#include "stats_collector.h"

static std::atomic<UbusExporter*> g_ubus_exporter_instance{nullptr};

namespace {
    static struct blob_buf bb;

    UbusExporter* get_exporter_instance() {
        return g_ubus_exporter_instance.load(std::memory_order_acquire);
    }

    static int ubus_get_stats_cb(struct ubus_context *ctx,
                                 struct ubus_object *obj,
                                 struct ubus_request_data *req,
                                 const char *method,
                                 struct blob_attr *msg) {
        (void)msg;

        UbusExporter* exporter = get_exporter_instance();
        if (!exporter) {
            syslog(LOG_ERR, "UBUS: get_stats_cb: exporter instance is null");
            return UBUS_STATUS_INVALID_ARGUMENT;
        }

        auto& stats = exporter->get_stats();

        blob_buf_init(&bb, 0);

        blobmsg_add_u64(&bb, "packets_received", stats.packets_received());
        blobmsg_add_u64(&bb, "bytes_received",   stats.bytes_received());
        blobmsg_add_u64(&bb, "errors",           stats.errors());

        int ret = ubus_send_reply(ctx, req, bb.head);
        if (ret != 0) {
            syslog(LOG_ERR, "ubus_send_reply failed: %d", ret);
        }
        blob_buf_free(&bb);

        return ret == 0 ? UBUS_STATUS_OK : UBUS_STATUS_UNKNOWN_ERROR;
    }
}

static const struct ubus_method light_control_methods[] = {
    UBUS_METHOD_NOARG("get_stats", ubus_get_stats_cb)
};

static struct ubus_object_type light_control_type = 
{
    .name = "light_control",
    .id = 0,
    .methods = light_control_methods,
    .n_methods = ARRAY_SIZE(light_control_methods)
};

struct ubus_object light_control_obj = {
    .name = "light_control",
    .type = &light_control_type,
    .methods = light_control_methods,
	.n_methods = ARRAY_SIZE(light_control_methods),
};

UbusExporter::UbusExporter(StatsCollector& stats, Logger& logger)
    : stats_(stats), logger_(logger) {
}

UbusExporter::~UbusExporter() {
    if (ctx_) {
        ubus_remove_object(ctx_, &light_control_obj);
        reset_context();
    }
}

// Вспомогательный метод (не static!) для настройки таймера
void UbusExporter::schedule_reconnect(const char* reason) {
    reconnect_timer_.cb = [](struct uloop_timeout* t) {
        UbusExporter* exp = get_exporter_instance();
        if (exp) {
            exp->retry_connect();
        } else {
            // Это редкий кейс: таймер сработал, но экземпляр уже уничтожен
            syslog(LOG_WARNING, "UBus reconnect timer fired but exporter instance is gone");
        }
    };
    
    uloop_timeout_set(&reconnect_timer_, 1000); // 1 секунда
    logger_.debug("%s, scheduled reconnect in 1 second", reason);
}

void UbusExporter::reset_context() {
    if (ctx_) {
        ubus_free(ctx_);
        ctx_ = nullptr;
        logger_.debug("UBus context freed and reset to nullptr");
    }
}

bool UbusExporter::init() {
    ctx_ = ubus_connect(NULL);
    if (!ctx_) {
        int err = errno;
        logger_.debug("UBus connect failed on init: %d (%s)", err, strerror(err));
        schedule_reconnect("Failed to connect to UBus");
        return false;
    }

    logger_.debug("UBus connected on first attempt");

    int ret = ubus_add_object(ctx_, &light_control_obj);
    if (ret == 0) {
        g_ubus_exporter_instance.store(this, std::memory_order_release);
        logger_.debug("UBus object 'light_control' registered successfully");
        return true;
    }

    logger_.error("ubus_add_object failed: %d (%m)", ret);
    
    reset_context();
    schedule_reconnect("Registration failed, will retry");

    return false;
}

void UbusExporter::retry_connect() {
    if (ctx_) {
        // Уже подключено — ничего не делаем
        return;
    }

    ctx_ = ubus_connect(NULL);
    if (!ctx_) {
        int err = errno;
        logger_.debug("UBus reconnect failed: %d (%s)", err, strerror(err));
        // Планируем следующую попытку
        uloop_timeout_set(&reconnect_timer_, 1000);
        return;
    }

    logger_.debug("UBus connected after retry");

    int ret = ubus_add_object(ctx_, &light_control_obj);
    if (ret == 0) {
        g_ubus_exporter_instance.store(this, std::memory_order_release);
        logger_.debug("UBus object 'light_control' registered successfully after retry");
        return;
    }

    logger_.error("ubus_add_object failed on retry: %d (%m)", ret);

    reset_context();
    uloop_timeout_set(&reconnect_timer_, 1000);
}

void UbusExporter::run(std::atomic<bool>& stop_flag) {
    logger_.debug("UBus run(): starting event loop");

    uloop_init();

   if (ctx_) {
        ubus_add_uloop(ctx_);
    } else {
        logger_.warn("UBus context is not ready yet; waiting for reconnect timer");
    }

    while (!stop_flag.load(std::memory_order_acquire)) {
        uloop_run_timeout(200);
    }
    uloop_done();
    logger_.debug("UBus event loop stopped");
}

void UbusExporter::stop() {
    uloop_end();
    g_ubus_exporter_instance.store(nullptr, std::memory_order_release);
    logger_.debug("UBus exporter cleaned up");
}

#endif
