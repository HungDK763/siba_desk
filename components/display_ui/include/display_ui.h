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

/**
 * @brief Cập nhật trạng thái kết nối BLE (hiển thị cột sóng hoặc dấu x đỏ ngay lập tức)
 */
esp_err_t display_ui_set_ble_connected(bool connected);

typedef void (*display_ui_mode_change_cb_t)(uint8_t mode);

/**
 * @brief Đăng ký callback khi người dùng chọn chuyển Mode trên màn hình UI
 */
void display_ui_register_mode_change_cb(display_ui_mode_change_cb_t cb);

/**
 * @brief Cập nhật trạng thái hiển thị của nút Mode (highlight nút tương ứng)
 */
esp_err_t display_ui_set_active_mode(uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_UI_H
