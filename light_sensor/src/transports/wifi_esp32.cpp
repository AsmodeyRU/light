#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sockets.h"

#include "transports/wifi_esp32.h"

static const char* TAG = "WifiEsp32Tr";

// Глобальный флаг для события IP (без аллокации)
static bool s_got_ip = false;

// C‑style обработчик событий (без лямбд, без захватов)
static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *event_data) {
    (void)arg;
    (void)event_data;
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_got_ip = true;
    }
}

WifiEsp32Transport::~WifiEsp32Transport() {
    if (_sock != -1) {
        close(_sock);
        _sock = -1;
    }
    // Wi‑Fi не останавливаем здесь — это ответственность верхнего уровня
}

bool WifiEsp32Transport::init() {
    if (_initialized) {
        return true;
    }

    if (!_ssid || !_pass || !_ip) {
        ESP_LOGW(TAG, "Missing SSID, password or IP in WifiEsp32Transport");
        return false;
    }

    // Уникальные имена, чтобы не конфликтовать с макросами ESP‑IDF
    constexpr size_t SSID_LIMIT = 32;
    constexpr size_t PASS_LIMIT = 64;

    if (strlen(_ssid) > SSID_LIMIT || strlen(_pass) > PASS_LIMIT) {
        ESP_LOGE(TAG, "SSID or password length is too long");
        return false;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %d", ret);
        return false;
    }
     
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    uint8_t mac_addr[6];
    ret = esp_wifi_get_mac(WIFI_IF_STA, mac_addr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_get_mac failed: %d", ret);
        esp_wifi_deinit();
        return false;
    }

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, _ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, _pass, sizeof(wifi_config.sta.password) - 1);

    // threshold.authmode обязателен, иначе подключение к WPA2 может не работать
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    // scan_method НЕ задаём: ESP‑IDF сам выполнит активный скан при подключении
    // use_nvs удалён: в новых версиях IDF это не задаётся здесь

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %d", ret);
        esp_wifi_deinit();
        return false;
    }

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
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

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr));

    s_got_ip = false;  // сброс флага перед ожиданием
    int retry = 0;
    while (!s_got_ip) {
        vTaskDelay(pdMS_TO_TICKS(500));
        retry++;
        if (retry > 20) {
            ESP_LOGE(TAG, "Wi‑Fi connect timeout: IP not obtained");
            esp_wifi_stop();
            esp_wifi_deinit();
            return false;
        }
    }

    _sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (_sock == -1) {
        ESP_LOGE(TAG, "socket() failed: %s", strerror(errno));
        esp_wifi_stop();
        esp_wifi_deinit();
        return false;
    }

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
    ESP_LOGI(TAG, "WifiEsp32Transport initialized, IP obtained");
    return true;
}

bool WifiEsp32Transport::send(const unsigned char* data, std::size_t len) {
    if (_sock == -1 && !init()) {
        // Ленивая инициализация
        return false;
    }

    ssize_t sent = sendto(_sock, data, len, 0,
                          reinterpret_cast<struct sockaddr*>(&_addr),
                          sizeof(_addr));

    return (sent == static_cast<ssize_t>(len));
}
