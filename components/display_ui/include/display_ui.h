#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "pc_sync.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Khởi tạo màn hình TFT 2.8" SPI (ILI9341) và engine đồ họa LVGL v8
 */
esp_err_t display_ui_init(void);

/**
 * @brief Cập nhật thông số Quota AI và Telemetry lên màn hình LVGL
 */
esp_err_t display_ui_update_quota(const pc_quota_packet_t *quota);

/**
 * @brief Điều khiển độ sáng đèn nền màn hình (0-100%)
 */
esp_err_t display_ui_set_backlight(uint8_t brightness_pct);

/**
 * @brief Chuyển đổi qua lại giữa màn hình Dashboard Quota và Home Clock
 */
esp_err_t display_ui_toggle_screen(void);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_UI_H
