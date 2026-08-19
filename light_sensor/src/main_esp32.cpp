#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "transports/transport_wrapper.hpp"
#include "config.hpp"
#include "app_core.h"


static const char* TAG = "app_esp32";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "ESP32 app started");

    TransportType transport("MY_WIFI", "MY_PASSWORD", "192.168.1.1", 5005);
    if (transport.init() == false)
        return;

    run_application(transport);
}