#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "ble_hid.h"
#include "launchpad_core.h"

static const char *TAG = "LP_CORE";

#define BOOT_BUTTON_GPIO GPIO_NUM_0

static uint16_t current_pad_mask = 0;

void launchpad_core_trigger_pad(uint8_t pad_index, bool pressed) {
    if (pad_index >= 16) return;

    if (pressed) {
        current_pad_mask |= (1 << pad_index);
    } else {
        current_pad_mask &= ~(1 << pad_index);
    }

    if (ble_hid_is_connected()) {
        // 1. Send Gamepad / LaunchPad pad state
        ble_hid_send_launchpad(current_pad_mask);

        // Pad 0 (BOOT button): Volume Up (Consumer Control Media Key)
        if (pad_index == 0) {
            if (pressed) {
                ble_hid_send_consumer(CONSUMER_VOLUME_UP);
                ESP_LOGI(TAG, "🔊 BOOT Button Pressed -> Sent: VOLUME UP");
            } else {
                ble_hid_send_consumer(0); // Release consumer key
                ESP_LOGI(TAG, "🔈 BOOT Button Released -> Released Media key");
            }
        }
    } else {
        ESP_LOGW(TAG, "Button pressed, but BLE Host is not connected yet.");
    }
}

static void launchpad_button_task(void *pvParameters) {
    bool last_state = true; // High due to pullup

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "LaunchPad Task Active. Monitoring BOOT button (GPIO %d)", BOOT_BUTTON_GPIO);

    while (1) {
        int current_level = gpio_get_level(BOOT_BUTTON_GPIO);
        bool pressed = (current_level == 0); // Active LOW

        if (pressed != (!last_state)) {
            last_state = !pressed;
            ESP_LOGI(TAG, "BOOT Button Event: %s", pressed ? "PRESSED" : "RELEASED");
            launchpad_core_trigger_pad(0, pressed);
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // 20ms debounce
    }
}

esp_err_t launchpad_core_init(void) {
    xTaskCreate(launchpad_button_task, "lp_btn_task", 3072, NULL, 10, NULL);
    return ESP_OK;
}
