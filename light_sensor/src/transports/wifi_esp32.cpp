#include <cstring>

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/sockets.h"

#include "transports/wifi_esp32.h"

namespace {
    constexpr const char* TAG = "WifiEsp32Tr";

    constexpr EventBits_t kGotIpBit = BIT0;
    constexpr EventBits_t kFailBit = BIT1;
    constexpr TickType_t kConnectTimeout = pdMS_TO_TICKS(30000);

    StaticEventGroup_t s_events_mem;
    EventGroupHandle_t s_events = nullptr;
}

static void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    auto* self = static_cast<WifiEsp32Transport*>(arg);

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        return;
    }
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        self->increment_attempt_number();
        ESP_LOGW(TAG, "Wi-Fi disconnected, retry %d", self->attempt_number());
        esp_wifi_connect();
        return;
    }
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(data);
        ESP_LOGI(TAG, "got ip " IPSTR, IP2STR(&event->ip_info.ip));

        self->clear_attempt_count();  // сбрасываем счетчик
        xEventGroupSetBits(s_events, kGotIpBit);
    }
}

WifiEsp32Transport::WifiEsp32Transport(const char* ssid, const char* pass, const char* ip, uint16_t port)
    : _ssid(ssid), _pass(pass), _ip(ip), _port(port), 
      _initialized(false), _stop_requested(false), _attempt(0)
{
    if (s_events == nullptr) {
        s_events = xEventGroupCreateStatic(&s_events_mem);
        if (s_events == nullptr) {
            ESP_LOGE(TAG, "Failed to create event group");
        }
    }
}

WifiEsp32Transport::~WifiEsp32Transport() {
    stop();
    if (_sock != -1) {
        close(_sock);
        _sock = -1;
    }
}

bool WifiEsp32Transport::init() {
    if (_initialized.load(std::memory_order_acquire)) {
        return true;
    }

    if (!_ssid || !_pass || !_ip) {
        ESP_LOGW(TAG, "Missing SSID, password or IP in WifiEsp32Transport");
        return false;
    }

    // Уникальные имена, чтобы не конфликтовать с макросами ESP‑IDF
    constexpr size_t SSID_LIMIT = 32;
    constexpr size_t PASS_LIMIT = 64;

    size_t ssid_len = strlen(_ssid);
    size_t pass_len = (_pass && _pass[0]) ? strlen(_pass) : 0;

    if (ssid_len > SSID_LIMIT || pass_len > PASS_LIMIT) {
        ESP_LOGE(TAG, "SSID or password too long");
        return false;
    }

    _stop_requested.store(false, std::memory_order_release);
    _attempt = 0;

    // Если задача уже запущена — ничего не делаем, она продолжит попытки
    if (_connect_task_handle != nullptr) {
        ESP_LOGD(TAG, "Connect task already running");
        return true;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_instance_register( WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register( IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, this, nullptr));

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %d", ret);
        return false;
    }

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    // Конфигурация Wi‑Fi
    wifi_config_t wifi_config = {};
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), _ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (_pass != nullptr) {
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), _pass, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    if (_pass == nullptr || !_pass[0]) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }

    wifi_config.sta.scan_method = WIFI_FAST_SCAN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(ret));
        return false;
    }

    // Запускаем задачу подключения
    const uint32_t stack_size = 4096;  // достаточно для Wi‑Fi + сокетов
    const UBaseType_t priority = 7;     // чуть выше среднего

    BaseType_t result = xTaskCreate(
        &wifi_connect_task,
        "wifi_conn",
        stack_size,          // размер стека в **байтах** 
        this,
        priority,
        &_connect_task_handle
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create wifi_connect_task");
        return false;
    }

    ESP_LOGI(TAG, "Wi‑Fi connect requested");
    return true;  // запрос принят, дальше работает задача
}

void WifiEsp32Transport::stop() {
    _stop_requested.store(true, std::memory_order_release);

    if (_connect_task_handle) {
        vTaskDelete(_connect_task_handle);
        _connect_task_handle = nullptr;
    }

    esp_wifi_stop();
    esp_wifi_deinit();

    xEventGroupClearBits(s_events, kGotIpBit | kFailBit);
    _initialized.store(false, std::memory_order_release);
}

bool WifiEsp32Transport::send(const unsigned char* data, std::size_t len) {
    if (!_initialized.load(std::memory_order_acquire) || _sock < 0) {
        ESP_LOGD(TAG, "Not initialized or socket invalid, drop send");
        return false;
    }
    const ssize_t sent = sendto(_sock, data, len, 0,
                                reinterpret_cast<struct sockaddr*>(&_addr),
                                sizeof(_addr));
    if (sent != static_cast<ssize_t>(len)) {
        ESP_LOGW(TAG, "sendto failed");
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
// Задача подключения (тяжёлый цикл)
// -----------------------------------------------------------------------------

void WifiEsp32Transport::wifi_connect_task(void* arg) {
    auto* self = static_cast<WifiEsp32Transport*>(arg);
    if (!self) {
        vTaskDelete(nullptr);
        return;
    }

    const TickType_t kRetryDelay = pdMS_TO_TICKS(2000);   // 2 сек между попытками
    const TickType_t kAttemptTimeout = pdMS_TO_TICKS(12000); // 12 сек на одну попытку

    while (!self->_stop_requested.load(std::memory_order_acquire)) {
        self->_attempt++;
        ESP_LOGI(TAG, "Wi‑Fi connect attempt #%lu", static_cast<unsigned long>(self->_attempt));

        xEventGroupClearBits(s_events, kGotIpBit | kFailBit);

        esp_err_t ret = esp_wifi_start();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(ret));
            esp_wifi_stop();
            vTaskDelay(kRetryDelay);
            continue;
        }
        // Ждём IP с таймаутом
        EventBits_t bits = xEventGroupWaitBits(s_events, kGotIpBit | kFailBit, pdFALSE, pdFALSE, kAttemptTimeout);

        bool got_ip = (bits & kGotIpBit);
        bool fail_bit = (bits & kFailBit);

        if (!got_ip) {
            ESP_LOGE(TAG, "Wi‑Fi attempt #%lu failed (timeout or error)",
                     static_cast<unsigned long>(self->_attempt));
            esp_wifi_stop();
            vTaskDelay(kRetryDelay);
            continue;
        }

        if (fail_bit) {
            // Можно дополнительно логировать причину из обработчика событий
            esp_wifi_stop();
            vTaskDelay(kRetryDelay);
            continue;
        }

        // Успешно получили IP — создаём сокет
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) {
            ESP_LOGE(TAG, "socket() failed: %s", strerror(errno));
            esp_wifi_stop();
            vTaskDelay(kRetryDelay);
            continue;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(self->_port);

        int rc = inet_pton(AF_INET, self->_ip, &addr.sin_addr);
        if (rc <= 0) {
            ESP_LOGE(TAG, "Invalid controller IP: %s", self->_ip);
            close(sock);
            esp_wifi_stop();
            vTaskDelay(kRetryDelay);
            continue;
        }

        // Финальная фиксация состояния
        self->_sock = sock;
        self->_addr = addr;
        self->_initialized.store(true, std::memory_order_release);
        self->_attempt.store(0, std::memory_order_release);

        ESP_LOGI(TAG, "Connected: UDP -> %s:%u", self->_ip, static_cast<unsigned>(self->_port));
        // Дальше задача просто живёт, чтобы можно было позже вызвать stop()
        break;
    }

    vTaskDelete(nullptr);
}