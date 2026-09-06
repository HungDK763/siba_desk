#include <stdio.h>
#include "esp_log.h"
#include "display_ui.h"

static const char *TAG = "DISPLAY_UI";

esp_err_t display_ui_init(void) {
    ESP_LOGI(TAG, "Display UI Component initialized (Placeholder ready)");
    return ESP_OK;
}

esp_err_t display_ui_update_quota(const display_quota_data_t *data) {
    return ESP_OK;
}

esp_err_t display_ui_update_gamepad(uint16_t buttons, int16_t lx, int16_t ly, int16_t rx, int16_t ry) {
    return ESP_OK;
}
