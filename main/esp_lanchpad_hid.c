#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "ble_hid.h"
#include "launchpad_core.h"

static const char *TAG = "MAIN_APP";

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Starting ESP32 BLE HID LaunchPad Controller...");

    // Initialize NVS (required for BLE bonding and security keys)
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize BLE HID Device Service Component
    ret = ble_hid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BLE HID component initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize LaunchPad Hardware Core Component
    ret = launchpad_core_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LaunchPad Core initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "ESP32 BLE HID LaunchPad Ready. Connect via Bluetooth on Ubuntu/Windows!");
}
