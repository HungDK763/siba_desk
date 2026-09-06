#ifndef PC_SYNC_H
#define PC_SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enum định nghĩa các trạng thái của module đồng bộ dữ liệu PC
 */
typedef enum {
    PC_SYNC_STATE_UNINITIALIZED = 0, // Chưa khởi tạo
    PC_SYNC_STATE_IDLE,              // Đang chờ dữ liệu từ PC
    PC_SYNC_STATE_RECEIVING,         // Đang nhận gói tin từ BLE
    PC_SYNC_STATE_PARSED_SUCCESS,    // Phân tích cú pháp JSON thành công
    PC_SYNC_STATE_ERROR_FORMAT,      // Lỗi định dạng JSON
    PC_SYNC_STATE_ERROR_INVALID_VAL  // Lỗi dữ liệu không hợp lệ
} pc_sync_state_t;

/**
 * @brief Enum mức độ cảnh báo của Quota AI
 */
typedef enum {
    QUOTA_ALERT_SAFE = 0,            // Quota an toàn (>= 50%) -> Xanh lá
    QUOTA_ALERT_WARNING,             // Quota cảnh báo (20% - 49%) -> Vàng
    QUOTA_ALERT_CRITICAL             // Quota nguy cấp (< 20%) -> Đỏ
} quota_alert_level_t;

/**
 * @brief Cấu trúc dữ liệu Quota & Telemetry nhận từ máy tính
 */
typedef struct {
    char model_name[32];             // Model đang dùng (ví dụ: Gemini 3.8 Flash)
    char time_vn[16];                // Giờ Việt Nam realtime (ví dụ: 01:21:12)
    char date_vn[32];                // Ngày tháng Việt Nam (ví dụ: Th 7 6 thg 9)
    char gemini_reset[16];           // Đếm ngược reset Gemini 5h (ví dụ: 04:45)
    char claude_reset[16];           // Đếm ngược reset Claude 5h (ví dụ: 04:59)
    uint8_t antigrav_quota_percent;  // Quota Gemini 5h còn lại (0 - 100%)
    uint8_t gemini_weekly_percent;   // Quota Gemini tuần (0 - 100%)
    uint8_t claude_5h_percent;       // Quota Claude 5h (0 - 100%)
    uint8_t claude_weekly_percent;   // Quota Claude tuần (0 - 100%)
    uint32_t prompt_count;           // Số lượng prompt / turn đã thực hiện
    uint8_t cpu_percent;             // % CPU máy tính
    uint8_t ram_percent;             // % RAM máy tính
    quota_alert_level_t alert_level; // Mức cảnh báo tự động tính
    uint32_t received_timestamp_ms;  // Thời gian nhận được gói tin (ms)
} pc_quota_packet_t;

/**
 * @brief Callback thông báo khi có dữ liệu Quota mới
 */
typedef void (*pc_quota_update_cb_t)(const pc_quota_packet_t *quota_data);

/**
 * @brief Khởi tạo module PC Sync
 * @param callback Hàm callback nhận thông báo khi có dữ liệu mới
 * @return ESP_OK nếu khởi tạo thành công
 */
esp_err_t pc_sync_init(pc_quota_update_cb_t callback);

/**
 * @brief Nhận và phân tích cú pháp gói tin thô nhận từ BLE GATT
 * @param raw_data Chuỗi byte nhận được (định dạng JSON)
 * @param len Độ dài chuỗi
 * @return ESP_OK nếu phân tích hợp lệ
 */
esp_err_t pc_sync_process_packet(const uint8_t *raw_data, size_t len);

/**
 * @brief Lấy trạng thái hiện tại của module PC Sync (dạng ENUM)
 */
pc_sync_state_t pc_sync_get_state(void);

/**
 * @brief Chuyển đổi trạng thái ENUM sang chuỗi string hiển thị
 */
const char* pc_sync_state_to_str(pc_sync_state_t state);

/**
 * @brief Lấy dữ liệu Quota mới nhất được lưu trong bộ đệm
 */
esp_err_t pc_sync_get_latest_quota(pc_quota_packet_t *out_data);

#ifdef __cplusplus
}
#endif

#endif // PC_SYNC_H
