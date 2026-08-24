#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "transports/transport_wrapper.hpp"
#include "config.hpp"
#include "sensors/bh1750.h"
#include "app_core.h"


namespace {
    constexpr const char* kTag = "app_esp32";
}  

extern "C" void app_main(void) {
    ESP_LOGI(kTag, "ESP32 app started");

    ESP_LOGI(kTag, "Wi‑Fi SSID: %s", WIFI_SSID);
    ESP_LOGD(kTag, "Target: %s:%u", CONTROLLER_IP, CONTROLLER_PORT);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    TransportType transport(WIFI_SSID, WIFI_PASS, CONTROLLER_IP, CONTROLLER_PORT);

    if (transport.init() == false) {
        ESP_LOGE(kTag, "Transport init failed");
        return;
    }

    ESP_LOGI(kTag, "Init requested. Waiting for connection...");
    while (transport.is_initialized() == false) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    Bh1750 sensor;
    if (!sensor.init(BH1750_I2C_ADDR)) {
        ESP_LOGE(kTag, "BH1750 init failed at 0x%02X SDA=%d SCL=%d",
                 static_cast<unsigned>(BH1750_I2C_ADDR), I2C_SDA_GPIO, I2C_SCL_GPIO);
        return;
    }

    run_application(transport, sensor);
}
