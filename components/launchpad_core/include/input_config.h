#ifndef INPUT_CONFIG_H
#define INPUT_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "ble_hid.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ==============================================================================
 * 📌 FILE CẤU HÌNH NÚT NHẤN & JOYSTICK (UNIVERSAL LAUNCHPAD HID)
 * ==============================================================================
 * Bạn có thể dễ dàng thay đổi nút nhấn ở đây để đổi chức năng:
 * - Khi ở GAMEPAD MODE: ánh xạ sang các nút Xbox Controller (A, B, X, Y, LB, RB, L3, v.v.)
 * - Khi ở KEYBOARD & MOUSE MODE: ánh xạ sang phím bàn phím hoặc click chuột
 * ==============================================================================
 */

// ==============================================================================
// 1. ĐỊNH NGHĨA CHÂN PHẦN CỨNG (GPIO PIN DEFINITIONS)
// ==============================================================================
#define PIN_JOY_VRX         GPIO_NUM_1   // Joystick VRx (ADC1_CH0)
#define PIN_JOY_VRY         GPIO_NUM_2   // Joystick VRy (ADC1_CH1)
#define PIN_JOY_SW          GPIO_NUM_42  // Joystick SW (Nút nhấn cần gạt - Chuột Trái Chọn)

#define PIN_BTN_1           GPIO_NUM_4   // Nút cơ 1 (J1-4)
#define PIN_BTN_2           GPIO_NUM_5   // Nút cơ 2 (J1-5)
#define PIN_BTN_3           GPIO_NUM_6   // Nút cơ 3 (J1-6)
#define PIN_BTN_4           GPIO_NUM_7   // Nút cơ 4 (J1-7)
#define PIN_BTN_5           GPIO_NUM_41  // Nút cơ 5 (J3-7) [Tùy chọn: GPIO 15 nếu đi cụm J1]
#define PIN_BTN_6           GPIO_NUM_40  // Nút cơ 6 (J3-8)

#define PIN_BOOT_BTN        GPIO_NUM_0   // Nút BOOT trên kit: Nhấn giữ 2s đổi mode, 6s xóa pairing

// ==============================================================================
// 2. CẤU HÌNH THÔNG SỐ JOYSTICK & CHUỘT
// ==============================================================================
#define JOY_ADC_UNIT        ADC_UNIT_1
#define JOY_ADC_CH_X        ADC_CHANNEL_0
#define JOY_ADC_CH_Y        ADC_CHANNEL_1

#define JOY_ADC_CENTER      2048         // Điểm giữa lý thuyết của ADC 12-bit (0 - 4095)
#define JOY_DEADZONE        220          // Vùng chết cho Gamepad mode (ADC diff)

#define JOY_SWAP_XY         true         // 🔄 HOÁN ĐỔI TRỤC: Trục X <-> Trục Y theo yêu cầu
#define JOY_INVERT_X        false        // Đảo chiều trục X (true / false)
#define JOY_INVERT_Y        true         // 🔄 Đảo chiều trục Y (chiều lên thành xuống, chiều xuống thành lên)

// --- CÁC THAM SỐ CẤU HÌNH CHUỘT (CHẾ ĐỘ KEYBOARD & MOUSE) ---
#define JOY_MOUSE_DEADZONE       50      // Biên ổn định gốc 0 (+-50/255) giảm mất ổn định tâm
#define JOY_MOUSE_MAX_SPEED      15.0f   // Tốc độ chuột tối đa khi gạt kịch biên (pixels / tick 10ms)
#define JOY_MOUSE_RETURN_THRESH  3       // Ngưỡng phát hiện nhả cần (hướng về home -> đứng yên) (thang 255)

#define JOY_POLL_INTERVAL_MS 10          // 10ms = 100Hz quét cực mượt chuẩn chuột cao cấp

// ==============================================================================
// 3. MÃ PHÍM BÀN PHÍM HID PHỔ BIẾN (KEYBOARD USAGE CODES)
// ==============================================================================
#ifndef HID_KEY_W
#define HID_KEY_W           0x1A
#define HID_KEY_S           0x16
#define HID_KEY_A           0x04
#define HID_KEY_D           0x07
#define HID_KEY_Q           0x14
#define HID_KEY_E           0x08
#define HID_KEY_R           0x15
#define HID_KEY_F           0x09
#define HID_KEY_Z           0x1D
#define HID_KEY_X           0x1B
#define HID_KEY_C           0x06
#define HID_KEY_V           0x19
#define HID_KEY_1           0x1E
#define HID_KEY_2           0x1F
#define HID_KEY_3           0x20
#define HID_KEY_4           0x21
#define HID_KEY_UP          0x52
#define HID_KEY_DOWN        0x51
#define HID_KEY_LEFT        0x50
#define HID_KEY_RIGHT       0x4F
#define HID_KEY_F1          0x3A
#define HID_KEY_F2          0x3B
#define HID_KEY_F3          0x3C
#define HID_KEY_F4          0x3D
#define HID_KEY_F5          0x3E
#define HID_KEY_PAGE_UP     0x4B
#define HID_KEY_PAGE_DOWN   0x4E
#define HID_KEY_HOME        0x4A
#define HID_KEY_END         0x4D
#endif

// ==============================================================================
// 4. KIỂU HÀNH ĐỘNG KHI Ở KEYBOARD & MOUSE MODE
// ==============================================================================
typedef enum {
    ACTION_TYPE_NONE = 0,
    ACTION_TYPE_KEYBOARD,      // Gửi phím bàn phím (+ modifier như Ctrl/Shift nếu có)
    ACTION_TYPE_MOUSE_BTN,     // Gửi click chuột (Trái, Phải, Giữa)
    ACTION_TYPE_CONSUMER,      // Gửi phím media (Tăng/Giảm âm lượng, Play/Pause)
} input_action_type_t;

// Cấu trúc mô tả một nút nhấn vật lý
typedef struct {
    const char *name;              // Tên định danh (vd: "BTN_1")
    gpio_num_t gpio;               // Chân GPIO kết nối
    
    // --- CẤU HÌNH CHO GAMEPAD MODE ---
    uint16_t gamepad_button;       // Bitmask nút Xbox (XBOX_BUTTON_A, B, X, Y, L1, R1, THUMBL,...)
    const char *gamepad_name;      // Tên hiển thị khi debug log

    // --- CẤU HÌNH CHO KEYBOARD & MOUSE MODE ---
    input_action_type_t kbm_type;  // Kiểu hành động (Bàn phím, Chuột, Media)
    uint8_t kbm_code;              // Mã phím HID hoặc Mã nút chuột
    uint8_t kbm_modifier;          // Phím bổ trợ (KEY_MOD_LCTRL, KEY_MOD_LSHIFT, v.v., hoặc 0)
    const char *kbm_name;          // Tên hiển thị khi debug log
} button_config_t;

#define TOTAL_CONFIG_BUTTONS 7     // 6 Nút vật lý (BTN 1 -> 6) + 1 Nút cần gạt (JOY_SW)

// ==============================================================================
// 5. BẢNG CẤU HÌNH ÁNH XẠ CÁC NÚT (BẠN TÙY CHỈNH TẠI ĐÂY)
// ==============================================================================
static const button_config_t BUTTON_CONFIGS[TOTAL_CONFIG_BUTTONS] = {
    // --------------------------------------------------------------------------
    // NÚT JOYSTICK SW (GPIO 42 - J3-6)
    // Gamepad: Nút A (Chọn / Tương tác)
    // KB/Mouse: CHUỘT TRÁI (CHỌN - Như nút TrackPoint trên máy ThinkPad!)
    // --------------------------------------------------------------------------
    {
        .name           = "JOY_SW",
        .gpio           = PIN_JOY_SW,
        .gamepad_button = XBOX_BUTTON_A,
        .gamepad_name   = "Xbox Button A (Select/Action)",
        .kbm_type       = ACTION_TYPE_MOUSE_BTN,
        .kbm_code       = MOUSE_BUTTON_LEFT,         // 👈 CHUỘT TRÁI (CHỌN)
        .kbm_modifier   = 0,
        .kbm_name       = "Mouse LEFT Click (Select / Chọn)",
    },

    // --------------------------------------------------------------------------
    // NÚT 1 (GPIO 4 - J1-4)
    // Gamepad: Nút A (Nhảy / Chọn)
    // KB/Mouse: Phím SPACEBAR (Dấu cách)
    // --------------------------------------------------------------------------
    {
        .name           = "BTN_1",
        .gpio           = PIN_BTN_1,
        .gamepad_button = XBOX_BUTTON_A,
        .gamepad_name   = "Xbox Button A (Jump/Select)",
        .kbm_type       = ACTION_TYPE_KEYBOARD,
        .kbm_code       = HID_KEY_SPACE,
        .kbm_modifier   = 0,
        .kbm_name       = "Keyboard SPACEBAR",
    },

    // --------------------------------------------------------------------------
    // NÚT 2 (GPIO 5 - J1-5)
    // Gamepad: Nút B (Hủy / Lùi / Crouch)
    // KB/Mouse: Phím ENTER
    // --------------------------------------------------------------------------
    {
        .name           = "BTN_2",
        .gpio           = PIN_BTN_2,
        .gamepad_button = XBOX_BUTTON_B,
        .gamepad_name   = "Xbox Button B (Cancel/Back)",
        .kbm_type       = ACTION_TYPE_KEYBOARD,
        .kbm_code       = HID_KEY_ENTER,
        .kbm_modifier   = 0,
        .kbm_name       = "Keyboard ENTER",
    },

    // --------------------------------------------------------------------------
    // NÚT 3 (GPIO 6 - J1-6)
    // Gamepad: Nút X (Tấn công / Tương tác)
    // KB/Mouse: Phím 'W' (Tiến lên)
    // --------------------------------------------------------------------------
    {
        .name           = "BTN_3",
        .gpio           = PIN_BTN_3,
        .gamepad_button = XBOX_BUTTON_X,
        .gamepad_name   = "Xbox Button X (Attack/Action)",
        .kbm_type       = ACTION_TYPE_KEYBOARD,
        .kbm_code       = HID_KEY_W,
        .kbm_modifier   = 0,
        .kbm_name       = "Keyboard 'W' (Up/Forward)",
    },

    // --------------------------------------------------------------------------
    // NÚT 4 (GPIO 7 - J1-7)
    // Gamepad: Nút Y (Đổi vũ khí / Menu)
    // KB/Mouse: Phím 'S' (Lùi lại)
    // --------------------------------------------------------------------------
    {
        .name           = "BTN_4",
        .gpio           = PIN_BTN_4,
        .gamepad_button = XBOX_BUTTON_Y,
        .gamepad_name   = "Xbox Button Y (Special/Menu)",
        .kbm_type       = ACTION_TYPE_KEYBOARD,
        .kbm_code       = HID_KEY_S,
        .kbm_modifier   = 0,
        .kbm_name       = "Keyboard 'S' (Down/Backward)",
    },

    // --------------------------------------------------------------------------
    // NÚT 5 (GPIO 41 - J3-7)
    // Gamepad: Nút L1 (Trái Bumper)
    // KB/Mouse: GIẢM ÂM LƯỢNG (Volume Down)
    // --------------------------------------------------------------------------
    {
        .name           = "BTN_5",
        .gpio           = PIN_BTN_5,
        .gamepad_button = XBOX_BUTTON_L1,
        .gamepad_name   = "Xbox Bumper L1",
        .kbm_type       = ACTION_TYPE_CONSUMER,
        .kbm_code       = CONSUMER_VOLUME_DOWN,
        .kbm_modifier   = 0,
        .kbm_name       = "Media Volume DOWN (Giảm âm lượng)",
    },

    // --------------------------------------------------------------------------
    // NÚT 6 (GPIO 40 - J3-8)
    // Gamepad: Nút R1 (Phải Bumper)
    // KB/Mouse: TĂNG ÂM LƯỢNG (Volume Up)
    // --------------------------------------------------------------------------
    {
        .name           = "BTN_6",
        .gpio           = PIN_BTN_6,
        .gamepad_button = XBOX_BUTTON_R1,
        .gamepad_name   = "Xbox Bumper R1",
        .kbm_type       = ACTION_TYPE_CONSUMER,
        .kbm_code       = CONSUMER_VOLUME_UP,
        .kbm_modifier   = 0,
        .kbm_name       = "Media Volume UP (Tăng âm lượng)",
    },
};

#ifdef __cplusplus
}
#endif

#endif // INPUT_CONFIG_H
