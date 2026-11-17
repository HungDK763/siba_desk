#ifndef ANALOG_JOYSTICK_H
#define ANALOG_JOYSTICK_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==============================================================================
// CẤU HÌNH TRỤC JOYSTICK & PHONG CÁCH THINKPAD TRACKPOINT
// ==============================================================================
#ifndef JOY_SWAP_XY
#define JOY_SWAP_XY         true   // 🔄 Hoán đổi trục: VRx -> Y, VRy -> X theo yêu cầu
#endif

#ifndef JOY_INVERT_X
#define JOY_INVERT_X        false  // Đảo chiều trục X nếu cần
#endif

#ifndef JOY_INVERT_Y
#define JOY_INVERT_Y        true   // 🔄 Mặc định đảo chiều trục Y (lên <-> xuống)
#endif

#ifndef JOY_MOUSE_DEADZONE
#define JOY_MOUSE_DEADZONE  50     // Biên ổn định gốc 0 (+-50/255)
#endif

#ifndef JOY_MOUSE_MAX_SPEED
#define JOY_MOUSE_MAX_SPEED 15.0f  // Tốc độ di chuột tối đa (pixels / tick)
#endif

#ifndef JOY_MOUSE_RETURN_THRESH
#define JOY_MOUSE_RETURN_THRESH 3  // Ngưỡng phát hiện hướng về home (thang 255)
#endif

typedef struct {
    int16_t x;       // -32768 (trái) .. 0 (giữa) .. 32767 (phải)
    int16_t y;       // -32768 (lên)   .. 0 (giữa) .. 32767 (xuống)
    uint16_t raw_x;  // Giá trị điện áp ADC thô (0 .. 4095)
    uint16_t raw_y;  // Giá trị điện áp ADC thô (0 .. 4095)
    bool button;     // Trạng thái nút nhấn cần gạt (true = đang nhấn, Active LOW)
} joystick_state_t;

// Cấu trúc cấu hình động cho Joystick & Chuột
typedef struct {
    bool swap_xy;
    bool invert_x;
    bool invert_y;
    int16_t mouse_deadzone;
    float mouse_max_speed;
    int16_t mouse_return_thresh;
} analog_joystick_config_t;

/**
 * @brief Khởi tạo Analog Joystick trên ADC1 (GPIO 1: VRx, GPIO 2: VRy, GPIO 42: SW)
 *        Tự động bật điện trở kéo lên nội (Internal Pull-Up) cho chân SW.
 */
esp_err_t analog_joystick_init(void);

/**
 * @brief Cập nhật cấu hình runtime cho Joystick & Chuột
 */
void analog_joystick_set_config(const analog_joystick_config_t *cfg);

/**
 * @brief Lấy cấu hình runtime hiện tại
 */
void analog_joystick_get_config(analog_joystick_config_t *out_cfg);

/**
 * @brief Đọc giá trị tọa độ cần gạt và trạng thái nút nhấn (kèm hoán đổi trục & deadzone)
 */
esp_err_t analog_joystick_read(joystick_state_t *out_state);

/**
 * @brief Chuyển đổi giá trị chuẩn hóa (-32768..32767) sang dải trục Xbox (0..65535, giữa 32768)
 */
uint16_t analog_joystick_to_xbox_axis(int16_t norm_val);

/**
 * @brief Tính toán delta di chuyển chuột với thuật toán:
 *        - Tốc độ tỉ lệ với góc lệch
 *        - Khi nhả cần (hướng đi về home) -> Đứng yên
 *        - Biên ổn định quanh gốc 0 (Deadzone)
 */
void analog_joystick_get_mouse_delta(const joystick_state_t *state, int8_t *out_dx, int8_t *out_dy);

#ifdef __cplusplus
}
#endif

#endif // ANALOG_JOYSTICK_H
