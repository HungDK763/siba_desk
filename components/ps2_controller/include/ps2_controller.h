#ifndef PS2_CONTROLLER_H
#define PS2_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// PS2 Button bitmasks
#define PS2_BTN_SELECT      (1 << 0)
#define PS2_BTN_L3          (1 << 1)
#define PS2_BTN_R3          (1 << 2)
#define PS2_BTN_START       (1 << 3)
#define PS2_BTN_UP          (1 << 4)
#define PS2_BTN_RIGHT       (1 << 5)
#define PS2_BTN_DOWN        (1 << 6)
#define PS2_BTN_LEFT        (1 << 7)
#define PS2_BTN_L2          (1 << 8)
#define PS2_BTN_R2          (1 << 9)
#define PS2_BTN_L1          (1 << 10)
#define PS2_BTN_R1          (1 << 11)
#define PS2_BTN_TRIANGLE    (1 << 12)
#define PS2_BTN_CIRCLE      (1 << 13)
#define PS2_BTN_CROSS       (1 << 14)
#define PS2_BTN_SQUARE      (1 << 15)

typedef struct {
    uint16_t buttons; // Bitmask of PS2_BTN_* (active high when pressed)
    uint8_t left_x;   // 0 (left) .. 128 (center) .. 255 (right)
    uint8_t left_y;   // 0 (up) .. 128 (center) .. 255 (down)
    uint8_t right_x;  // 0 (left) .. 128 (center) .. 255 (right)
    uint8_t right_y;  // 0 (up) .. 128 (center) .. 255 (down)
} ps2_state_t;

/**
 * @brief Khởi tạo giao tiếp với cổng cắm tay cầm PS2
 */
esp_err_t ps2_controller_init(void);

/**
 * @brief Đọc trạng thái nút bấm và cần analog của tay cầm PS2
 */
esp_err_t ps2_controller_read(ps2_state_t *out_state);

#ifdef __cplusplus
}
#endif

#endif // PS2_CONTROLLER_H
