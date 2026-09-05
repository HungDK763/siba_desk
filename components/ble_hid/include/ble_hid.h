#ifndef BLE_HID_H
#define BLE_HID_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// HID Report IDs
#define HID_REPORT_ID_KEYBOARD      1
#define HID_REPORT_ID_CONSUMER      2
#define HID_REPORT_ID_LAUNCHPAD     3
#define HID_REPORT_ID_MOUSE         4

// Keyboard Modifier Bitmasks
#define KEYBOARD_MODIFIER_LEFTCTRL   (1 << 0)
#define KEYBOARD_MODIFIER_LEFTSHIFT  (1 << 1)
#define KEYBOARD_MODIFIER_LEFTALT    (1 << 2)
#define KEYBOARD_MODIFIER_LEFTGUI    (1 << 3)
#define KEYBOARD_MODIFIER_RIGHTCTRL  (1 << 4)
#define KEYBOARD_MODIFIER_RIGHTSHIFT (1 << 5)
#define KEYBOARD_MODIFIER_RIGHTALT   (1 << 6)
#define KEYBOARD_MODIFIER_RIGHTGUI   (1 << 7)

// Common HID Keycodes (Usage Page 0x07)
#define HID_KEY_RESERVED            0x00
#define HID_KEY_A                   0x04
#define HID_KEY_B                   0x05
#define HID_KEY_C                   0x06
#define HID_KEY_SPACE               0x2C
#define HID_KEY_ENTER               0x28
#define HID_KEY_ESCAPE              0x29
#define HID_KEY_R                   0x15 // Record in DAWs
#define HID_KEY_RIGHT_ARROW         0x4F
#define HID_KEY_LEFT_ARROW          0x50
#define HID_KEY_DOWN_ARROW          0x51
#define HID_KEY_UP_ARROW            0x52

// Mouse Button Bitmasks
#define MOUSE_BUTTON_LEFT           (1 << 0)
#define MOUSE_BUTTON_RIGHT          (1 << 1)
#define MOUSE_BUTTON_MIDDLE         (1 << 2)

// Consumer Control Usages Bitmasks
#define CONSUMER_SCAN_NEXT          (1 << 0)
#define CONSUMER_SCAN_PREV          (1 << 1)
#define CONSUMER_STOP               (1 << 2)
#define CONSUMER_PLAY_PAUSE         (1 << 3)
#define CONSUMER_MUTE               (1 << 4)
#define CONSUMER_VOLUME_UP          (1 << 5)
#define CONSUMER_VOLUME_DOWN        (1 << 6)

// Keyboard Report Struct (Standard 8-byte Boot & Generic HID report)
typedef struct {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t key_codes[6];
} __attribute__((packed)) keyboard_report_t;

// Mouse Report Struct (4-byte Standard HID mouse report)
typedef struct {
    uint8_t buttons;  // Bit 0: Left, Bit 1: Right, Bit 2: Middle
    int8_t x;         // Relative movement X (-127 to 127)
    int8_t y;         // Relative movement Y (-127 to 127)
    int8_t wheel;     // Relative scroll wheel (-127 to 127)
} __attribute__((packed)) mouse_report_t;

/**
 * @brief Initialize BLE HID Device Service, SMP Bonding, and GAP Advertising
 */
esp_err_t ble_hid_init(void);

/**
 * @brief Check if BLE HID device is connected to a host (Windows/Ubuntu/macOS/Android)
 */
bool ble_hid_is_connected(void);

/**
 * @brief Send Keyboard Report over BLE
 * @param modifier Bitmask of modifier keys (Ctrl, Shift, Alt, GUI)
 * @param key_codes Array of up to 6 simultaneous key codes
 * @param key_count Number of keys in key_codes array
 */
esp_err_t ble_hid_send_keyboard(uint8_t modifier, const uint8_t *key_codes, uint8_t key_count);

/**
 * @brief Send Mouse Report over BLE
 * @param buttons Bitmask of mouse buttons (MOUSE_BUTTON_LEFT, etc.)
 * @param x Relative displacement X (-127 to 127)
 * @param y Relative displacement Y (-127 to 127)
 * @param wheel Relative scroll wheel movement (-127 to 127)
 */
esp_err_t ble_hid_send_mouse(uint8_t buttons, int8_t x, int8_t y, int8_t wheel);

/**
 * @brief Send Consumer Control Media Key Report over BLE
 * @param consumer_bitmask Bitmask of CONSUMER_* definitions
 */
esp_err_t ble_hid_send_consumer(uint8_t consumer_bitmask);

/**
 * @brief Send LaunchPad 16-pad state report over BLE
 * @param pad_mask Bitmask of 16 buttons
 */
esp_err_t ble_hid_send_launchpad(uint16_t pad_mask);

#ifdef __cplusplus
}
#endif

#endif // BLE_HID_H
