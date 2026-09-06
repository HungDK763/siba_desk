#include <stdio.h>
#include "esp_log.h"
#include "ps2_controller.h"

static const char *TAG = "PS2_CTRL";

esp_err_t ps2_controller_init(void) {
    ESP_LOGI(TAG, "PS2 Controller Component initialized (Placeholder ready)");
    return ESP_OK;
}

esp_err_t ps2_controller_read(ps2_state_t *out_state) {
    if (!out_state) return ESP_ERR_INVALID_ARG;
    out_state->buttons = 0;
    out_state->left_x = 128;
    out_state->left_y = 128;
    out_state->right_x = 128;
    out_state->right_y = 128;
    return ESP_OK;
}
