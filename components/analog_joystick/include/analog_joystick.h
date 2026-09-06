#ifndef ANALOG_JOYSTICK_H
#define ANALOG_JOYSTICK_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int16_t x;       // -32768 (trái) .. 0 (giữa) .. 32767 (phải)
    int16_t y;       // -32768 (lên)  .. 0 (giữa) .. 32767 (xuống)
    uint16_t raw_x;  // Giá trị điện áp ADC thô (0 .. 4095)
    uint16_t raw_y;  // Giá trị điện áp ADC thô (0 .. 4095)
    bool button;     // Trạng thái nút nhấn cần gạt (true = đang nhấn)
} joystick_state_t;

/**
 * @brief Khởi tạo Analog Joystick trên ADC1 (GPIO 1: VRx, GPIO 2: VRy, GPIO 3: SW)
 */
esp_err_t analog_joystick_init(void);

/**
 * @brief Đọc giá trị tọa độ cần gạt và trạng thái nút nhấn
 */
esp_err_t analog_joystick_read(joystick_state_t *out_state);

#ifdef __cplusplus
}
#endif

#endif // ANALOG_JOYSTICK_H
