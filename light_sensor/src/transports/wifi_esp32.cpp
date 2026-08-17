#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sockets.h"

#include "transports/wifi_esp32.h"

static const char* TAG = "WifiEsp32Tr";

WifiEsp32Transport::~WifiEsp32Transport() {
    if (_sock != -1) {
        close(_sock);
        _sock = -1;
    }
    // Отключение Wi‑Fi обычно делается на уровне приложения/системы,
    // а не внутри транспорта, чтобы не ломать другие модули.
}

bool WifiEsp32Transport::init() {
    // Защита от повторной инициализации (тяжёлое подключение к Wi‑Fi)
    if (_initialized) {
        return true;
    }

    if (!_ssid || !_ip) {
        ESP_LOGW(TAG, "Missing SSID or IP in WifiEsp32Transport");
        return false;
    }

    // --- Инициализация Wi‑Fi (ESP‑IDF) ---
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %d", ret);
        return false;
    }

    uint8_t mac_addr[6];
    ret = esp_wifi_get_mac(ESP_IF_WIFI_STA, mac_addr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_get_mac failed: %d", ret);
        esp_wifi_deinit();
        return false;
    }

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, _ssid);
    strcpy((char*)wifi_config.sta.password, _pass);
    wifi_config.sta.scan_method = WIFI_SCAN_TYPE_ACTIVE;
    wifi_config.sta.scan_time.active.min = 100;
    wifi_config.sta.scan_time.active.max = 300;

    ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %d", ret);
        esp_wifi_deinit();
        return false;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %d", ret);
        esp_wifi_deinit();
        return false;
    }

    // Блокирующее ожидание подключения (в реальном проекте лучше через события)
    int retry = 0;
    while (true) {
        if (wifi_is_connected()) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        retry++;
        if (retry > 20) {
            ESP_LOGE(TAG, "Wi‑Fi connect timeout");
            esp_wifi_stop();
            esp_wifi_deinit();
            return false;
        }
    }
    // --------------------------------------

    // Создание UDP‑сокета
    _sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (_sock == -1) {
        ESP_LOGE(TAG, "socket() failed");
        esp_wifi_stop();
        esp_wifi_deinit();
        return false;
    }

    // _addr уже обнулён благодаря {} в объявлении поля
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(_port);

    if (inet_pton(AF_INET, _ip, &_addr.sin_addr) <= 0) {
        ESP_LOGE(TAG, "Invalid IP address");
        close(_sock);
        _sock = -1;
        esp_wifi_stop();
        esp_wifi_deinit();
        return false;
    }

    _initialized = true;
    ESP_LOGI(TAG, "WifiEsp32Transport initialized");
    return true;
}

bool WifiEsp32Transport::send(const unsigned char* data, std::size_t len) {
    if (_sock == -1 && !init()) {
        // Ленивая инициализация: если забыли вызвать init(), пробуем сделать это здесь
        return false;
    }

    ssize_t sent = sendto(_sock, data, len, 0,
                          reinterpret_cast<struct sockaddr*>(&_addr),
                          sizeof(_addr));

    return (sent == static_cast<ssize_t>(len));
}
