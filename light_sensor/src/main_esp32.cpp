#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "transports/transport_wrapper.hpp"
#include "config.hpp"
#include "app_core.h"

static const char* TAG = "app_esp32";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "ESP32 app started");

    ESP_LOGI(TAG, "Wi‑Fi SSID: %s", WIFI_SSID);
    ESP_LOGD(TAG, "Target: %s:%u", CONTROLLER_IP, CONTROLLER_PORT);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // Конструктор тоже принимает безопасные constexpr, а не сырые CONFIG_*
    TransportType transport(WIFI_SSID,
                            WIFI_PASS,
                            CONTROLLER_IP,
                            CONTROLLER_PORT);

    if (!transport.init()) {
        ESP_LOGE(TAG, "Transport init failed");
        return;
    }

    run_application(transport);
}
