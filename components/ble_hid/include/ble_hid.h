#ifndef BLE_HID_H
#define BLE_HID_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// HID Report IDs
#define HID_REPORT_ID_GAMEPAD       1 // Xbox Gamepad Report
#define HID_REPORT_ID_KEYBOARD      2 // Standard 8-byte Keyboard Report
#define HID_REPORT_ID_MOUSE         3 // Standard Mouse Report
#define HID_REPORT_ID_CONSUMER      4 // Consumer Control (Media)
#define HID_REPORT_ID_LAUNCHPAD     HID_REPORT_ID_GAMEPAD

// Xbox Controller Button Bitmasks (Native to Bluepad32 uni_hid_parser_xboxone)
#define XBOX_BUTTON_A           (1 << 0)
#define XBOX_BUTTON_B           (1 << 1)
#define XBOX_BUTTON_X           (1 << 3)
#define XBOX_BUTTON_Y           (1 << 4)
#define XBOX_BUTTON_L1          (1 << 6)
#define XBOX_BUTTON_R1          (1 << 7)
#define XBOX_BUTTON_SELECT      (1 << 10)
#define XBOX_BUTTON_START       (1 << 11)
#define XBOX_BUTTON_HOME        (1 << 12)
#define XBOX_BUTTON_THUMBL      (1 << 13) // L3
#define XBOX_BUTTON_THUMBR      (1 << 14) // R3

// D-Pad Hat Switch
#define GAMEPAD_DPAD_CENTER     0
#define GAMEPAD_DPAD_UP         1
#define GAMEPAD_DPAD_UP_RIGHT   2
#define GAMEPAD_DPAD_RIGHT      3
#define GAMEPAD_DPAD_DOWN_RIGHT 4
#define GAMEPAD_DPAD_DOWN       5
#define GAMEPAD_DPAD_DOWN_LEFT  6
#define GAMEPAD_DPAD_LEFT       7
#define GAMEPAD_DPAD_UP_LEFT    8

// Mouse Buttons
#define MOUSE_BUTTON_NONE       0x00
#define MOUSE_BUTTON_LEFT       (1 << 0)
#define MOUSE_BUTTON_RIGHT      (1 << 1)
#define MOUSE_BUTTON_MIDDLE     (1 << 2)

// Keyboard Modifiers
#define KEY_MOD_LCTRL           (1 << 0)
#define KEY_MOD_LSHIFT          (1 << 1)
#define KEY_MOD_LALT            (1 << 2)
#define KEY_MOD_LMETA           (1 << 3)
#define KEY_MOD_RCTRL           (1 << 4)
#define KEY_MOD_RSHIFT          (1 << 5)
#define KEY_MOD_RALT            (1 << 6)
#define KEY_MOD_RMETA           (1 << 7)

// Common Key Codes (HID Usage Tables)
#define HID_KEY_RESERVED        0x00
#define HID_KEY_A               0x04
#define HID_KEY_B               0x05
#define HID_KEY_C               0x06
#define HID_KEY_D               0x07
#define HID_KEY_E               0x08
#define HID_KEY_ENTER           0x28
#define HID_KEY_ESCAPE          0x29
#define HID_KEY_BACKSPACE       0x2A
#define HID_KEY_TAB             0x2B
#define HID_KEY_SPACE           0x2C

// Consumer Control Usages (Media)
#define CONSUMER_VOLUME_UP      (1 << 0)
#define CONSUMER_VOLUME_DOWN    (1 << 1)
#define CONSUMER_MUTE           (1 << 2)
#define CONSUMER_PLAY_PAUSE     (1 << 3)
#define CONSUMER_SCAN_NEXT      (1 << 4)
#define CONSUMER_SCAN_PREV      (1 << 5)

// Xbox Wireless Controller Input Report Struct (16 bytes)
typedef struct {
    uint16_t left_x;      // 0 to 65535 (center: 32768)
    uint16_t left_y;      // 0 to 65535 (center: 32768, UP MAX: 0, DOWN MAX: 65535)
    uint16_t right_x;     // 0 to 65535 (center: 32768)
    uint16_t right_y;     // 0 to 65535 (center: 32768)
    uint16_t brake;       // 0 to 1023 (L2 Trigger: 1023 = 100% max)
    uint16_t accelerator; // 0 to 1023 (R2 Trigger: 1023 = 100% max)
    uint8_t  dpad;        // 0 = released, 1-8 = directions
    uint16_t buttons;     // Bitmask of XBOX_BUTTON_*
    uint8_t  record;      // Share button (0)
} __attribute__((packed)) xbox_gamepad_report_t;

// Standard Keyboard Input Report Struct (8 bytes)
typedef struct {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t key_codes[6];
} __attribute__((packed)) keyboard_report_t;

// Standard Mouse Input Report Struct (4 bytes)
typedef struct {
    uint8_t buttons;
    int8_t  x;
    int8_t  y;
    int8_t  wheel;
} __attribute__((packed)) mouse_report_t;

/**
 * @brief Initialize BLE HID Device Service, SMP Bonding, and GAP Advertising
 */
esp_err_t ble_hid_init(void);

/**
 * @brief Check if BLE HID device is connected to a host
 */
bool ble_hid_is_connected(void);

/**
 * @brief Clear all paired bond devices from NVS and restart advertising for new pairing
 */
esp_err_t ble_hid_clear_bonds(void);

/**
 * @brief Send Xbox Gamepad Report over BLE
 */
esp_err_t ble_hid_send_xbox_gamepad(uint16_t left_x, uint16_t left_y,
                                    uint16_t right_x, uint16_t right_y,
                                    uint16_t brake, uint16_t accelerator,
                                    uint16_t buttons, uint8_t dpad);

/**
 * @brief Send simple Gamepad Report (-127 to 127) mapped to Xbox controller
 */
esp_err_t ble_hid_send_gamepad(int8_t left_x, int8_t left_y, int8_t right_x, int8_t right_y,
                               uint8_t brake, uint8_t throttle,
                               uint16_t buttons, uint8_t dpad);

/**
 * @brief Send Standard Keyboard Report (Single key press/release)
 */
esp_err_t ble_hid_send_keyboard(uint8_t modifier, uint8_t key_code);

/**
 * @brief Send Standard Keyboard Report (Up to 6 simultaneous keys)
 */
esp_err_t ble_hid_send_keyboard_report(uint8_t modifier, const uint8_t *key_codes, uint8_t key_count);

/**
 * @brief Send Standard Mouse Report
 */
esp_err_t ble_hid_send_mouse(uint8_t buttons, int8_t x, int8_t y, int8_t wheel);

/**
 * @brief Send Consumer Control Report (Media keys)
 */
esp_err_t ble_hid_send_consumer(uint8_t consumer_bitmask);

/**
 * @brief Backward compatible wrapper
 */
esp_err_t ble_hid_send_launchpad(uint16_t pad_mask);

// Callback type for custom PC Sync data received over BLE GATT
typedef void (*ble_pc_sync_rx_cb_t)(const uint8_t *data, size_t len);

/**
 * @brief Register callback for PC Sync Quota data received over BLE
 */
void ble_hid_register_pc_sync_cb(ble_pc_sync_rx_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif // BLE_HID_H
