#include <stdio.h>
#include "esp_log.h"
#include "analog_joystick.h"

static const char *TAG = "JOYSTICK";

esp_err_t analog_joystick_init(void) {
    ESP_LOGI(TAG, "Analog Joystick Component initialized on GPIO 1 (VRx), GPIO 2 (VRy), GPIO 3 (SW) - Placeholder ready");
    return ESP_OK;
}

esp_err_t analog_joystick_read(joystick_state_t *out_state) {
    if (!out_state) return ESP_ERR_INVALID_ARG;
    out_state->x = 0;
    out_state->y = 0;
    out_state->raw_x = 2048;
    out_state->raw_y = 2048;
    out_state->button = false;
    return ESP_OK;
}
