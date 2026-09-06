#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t claude_quota_percent;
    uint8_t antigrav_quota_percent;
    uint8_t cpu_usage_percent;
    uint8_t ram_usage_percent;
    uint8_t reset_hours_left;
} display_quota_data_t;

/**
 * @brief Khởi tạo màn hình TFT 2.8" SPI và bộ điều khiển cảm ứng
 */
esp_err_t display_ui_init(void);

/**
 * @brief Cập nhật thông số Quota AI lên màn hình
 */
esp_err_t display_ui_update_quota(const display_quota_data_t *data);

/**
 * @brief Cập nhật hiển thị trạng thái nút bấm / analog của tay PS2
 */
esp_err_t display_ui_update_gamepad(uint16_t buttons, int16_t lx, int16_t ly, int16_t rx, int16_t ry);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_UI_H
