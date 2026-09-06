#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "ble_hid.h"
#include "pc_sync.h"
#include "display_ui.h"
#include "launchpad_core.h"

static const char *TAG = "LP_CORE";

#define BOOT_BUTTON_GPIO GPIO_NUM_0
#define LED_STRIP_GPIO   GPIO_NUM_48
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

#define MODE_SWITCH_TICKS pdMS_TO_TICKS(2000) // 2 seconds to switch mode
#define BOND_CLEAR_TICKS  pdMS_TO_TICKS(6000) // 6 seconds to clear bonds

typedef enum {
    LP_MODE_GAMEPAD = 0,
    LP_MODE_KEYBOARD,
    LP_MODE_MOUSE,
    LP_MODE_MAX
} launchpad_mode_t;

static launchpad_mode_t current_mode = LP_MODE_GAMEPAD;
static const char *mode_names[LP_MODE_MAX] = {
    "GAMEPAD 🎮 (Nút A + Cần trái LÊN MAX / PC & Bluepad32)",
    "KEYBOARD ⌨️ (Phím Space / Trình duyệt & Game)",
    "MOUSE 🖱️ (Click Chuột Trái / Left Click)"
};

static uint16_t current_pad_mask = 0;
static led_strip_handle_t led_strip = NULL;

static void launchpad_led_init(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO,
        .max_leds = 1,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
        .flags.with_dma = false,
    };

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✨ WS2812 RGB LED initialized on GPIO %d", LED_STRIP_GPIO);
    } else {
        ESP_LOGE(TAG, "❌ Failed to initialize WS2812 LED: %s", esp_err_to_name(ret));
    }
}

static void launchpad_set_led_mode(launchpad_mode_t mode) {
    if (!led_strip) return;
    switch (mode) {
        case LP_MODE_GAMEPAD:
            // 🔴 RED = Gamepad Mode
            led_strip_set_pixel(led_strip, 0, 40, 0, 0);
            break;
        case LP_MODE_KEYBOARD:
            // 🟢 GREEN = Keyboard Mode
            led_strip_set_pixel(led_strip, 0, 0, 40, 0);
            break;
        case LP_MODE_MOUSE:
            // 🔵 BLUE = Mouse Mode
            led_strip_set_pixel(led_strip, 0, 0, 0, 40);
            break;
        default:
            break;
    }
    led_strip_refresh(led_strip);
}

static void launchpad_flash_led_bond_clear(void) {
    if (!led_strip) return;
    for (int i = 0; i < 4; i++) {
        led_strip_set_pixel(led_strip, 0, 40, 40, 0); // 🟡 Yellow flash
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(100));
        led_strip_clear(led_strip);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    launchpad_set_led_mode(current_mode);
}

static void launchpad_release_all_inputs(void) {
    if (ble_hid_is_connected()) {
        switch (current_mode) {
            case LP_MODE_GAMEPAD:
                ble_hid_send_xbox_gamepad(32768, 32768, 32768, 32768, 0, 0, 0, GAMEPAD_DPAD_CENTER);
                break;
            case LP_MODE_KEYBOARD:
                ble_hid_send_keyboard(0, 0);
                break;
            case LP_MODE_MOUSE:
                ble_hid_send_mouse(0, 0, 0, 0);
                break;
            default:
                break;
        }
    }
}

void launchpad_core_trigger_pad(uint8_t pad_index, bool pressed) {
    if (pad_index >= 16) return;

    if (pressed) {
        current_pad_mask |= (1 << pad_index);
    } else {
        current_pad_mask &= ~(1 << pad_index);
    }

    if (ble_hid_is_connected()) {
        if (pad_index == 0) {
            switch (current_mode) {
                case LP_MODE_GAMEPAD:
                    if (pressed) {
                        // Send: Button A (0x0001) + Left Stick UP MAX (left_y = 0) + Trigger L2 (1023) + D-pad UP
                        // This triggers Gamepad API on web browser (gamepad-tester.com), Windows joy.cpl, and Bluepad32!
                        ble_hid_send_xbox_gamepad(32768, 0, 32768, 32768, 1023, 0, XBOX_BUTTON_A, GAMEPAD_DPAD_UP);
                        ESP_LOGI(TAG, "🕹️ [GAMEPAD] BOOT Pressed -> Nút A + Cần trái LÊN MAX + Cò L2 + D-Pad LÊN");
                    } else {
                        ble_hid_send_xbox_gamepad(32768, 32768, 32768, 32768, 0, 0, 0, GAMEPAD_DPAD_CENTER);
                        ESP_LOGI(TAG, "🕹️ [GAMEPAD] BOOT Released -> Nhả toàn bộ nút & về giữa (Center)");
                    }
                    break;

                case LP_MODE_KEYBOARD:
                    if (pressed) {
                        ble_hid_send_keyboard(0, HID_KEY_SPACE);
                        ESP_LOGI(TAG, "⌨️ [KEYBOARD] BOOT Pressed -> Gửi phím SPACE (Phím Cách)");
                    } else {
                        ble_hid_send_keyboard(0, 0);
                        ESP_LOGI(TAG, "⌨️ [KEYBOARD] BOOT Released -> Nhả phím (Key Released)");
                    }
                    break;

                case LP_MODE_MOUSE:
                    if (pressed) {
                        ble_hid_send_mouse(MOUSE_BUTTON_LEFT, 0, 0, 0);
                        ESP_LOGI(TAG, "🖱️ [MOUSE] BOOT Pressed -> Click Chuột Trái (Left Click Down)");
                    } else {
                        ble_hid_send_mouse(0, 0, 0, 0);
                        ESP_LOGI(TAG, "🖱️ [MOUSE] BOOT Released -> Nhả Chuột Trái (Left Click Released)");
                    }
                    break;

                default:
                    break;
            }
        } else {
            // Other pads
            ble_hid_send_launchpad(current_pad_mask);
        }
    } else {
        ESP_LOGW(TAG, "Nút được bấm, nhưng chưa có host BLE kết nối.");
    }
}

static void launchpad_button_task(void *pvParameters) {
    bool last_state = true; // High due to pullup
    TickType_t press_start_time = 0;
    bool mode_switched = false;
    bool bond_cleared = false;

    // Initialize RGB LED on GPIO 48
    launchpad_led_init();
    launchpad_set_led_mode(current_mode);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "LaunchPad Task Active. Monitoring BOOT button (GPIO %d)", BOOT_BUTTON_GPIO);
    ESP_LOGI(TAG, "Mode mặc định: %s", mode_names[current_mode]);
    ESP_LOGI(TAG, "💡 HƯỚNG DẪN:");
    ESP_LOGI(TAG, "   - Nhấn nhanh (<2s): Kích hoạt tính năng theo Mode hiện tại.");
    ESP_LOGI(TAG, "   - Giữ 2 giây: Chuyển sang Mode tiếp theo (🔴 Gamepad -> 🟢 Keyboard -> 🔵 Mouse).");
    ESP_LOGI(TAG, "   - Giữ >6 giây: Xóa bộ nhớ ghép đôi Bluetooth để kết nối máy mới.");

    while (1) {
        int current_level = gpio_get_level(BOOT_BUTTON_GPIO);
        bool pressed = (current_level == 0); // Active LOW

        if (pressed && last_state) {
            // Button just pressed down
            last_state = false;
            press_start_time = xTaskGetTickCount();
            mode_switched = false;
            bond_cleared = false;
            launchpad_core_trigger_pad(0, true);
        } else if (pressed && !last_state) {
            // Button is being held down
            TickType_t duration = xTaskGetTickCount() - press_start_time;

            // Check 2 seconds: Switch Mode
            if (!mode_switched && duration >= MODE_SWITCH_TICKS) {
                mode_switched = true;
                launchpad_release_all_inputs();

                current_mode = (current_mode + 1) % LP_MODE_MAX;
                launchpad_set_led_mode(current_mode);
                ESP_LOGI(TAG, "==================================================");
                ESP_LOGI(TAG, "🔀 ĐÃ GIỮ 2 GIÂY -> CHUYỂN SANG MODE: %s", mode_names[current_mode]);
                ESP_LOGI(TAG, "==================================================");
            }

            // Check 6 seconds: Clear Bonds
            if (!bond_cleared && duration >= BOND_CLEAR_TICKS) {
                bond_cleared = true;
                ESP_LOGW(TAG, "==================================================");
                ESP_LOGW(TAG, "🔄 ĐÃ GIỮ >6 GIÂY -> XÓA BỘ NHỚ PAIRING ĐỂ GHÉP ĐÔI MỚI!");
                ESP_LOGW(TAG, "==================================================");
                launchpad_flash_led_bond_clear();
                ble_hid_clear_bonds();
            }
        } else if (!pressed && !last_state) {
            // Button just released
            last_state = true;
            if (!mode_switched) {
                launchpad_core_trigger_pad(0, false);
            } else {
                ESP_LOGI(TAG, "BOOT button nhả ra sau khi chuyển mode.");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // 20ms debounce
    }
}

#define BOX_WIDTH 66

static int get_visual_width(const char *str) {
    int width = 0;
    const uint8_t *p = (const uint8_t *)str;
    while (*p) {
        if (*p < 0x80) {
            width += 1;
            p++;
        } else if ((*p & 0xE0) == 0xC0) {
            width += 1;
            p += 2;
        } else if ((*p & 0xF0) == 0xE0) {
            width += 1;
            p += 3;
        } else if ((*p & 0xF8) == 0xF0) {
            width += 2; // 4-byte UTF-8 emoji (🟢, 🟡, 🔴)
            p += 4;
        } else {
            width += 1;
            p++;
        }
    }
    return width;
}

static void print_box_line(const char *content) {
    int vis_w = get_visual_width(content);
    int pad = BOX_WIDTH - vis_w;
    if (pad < 0) pad = 0;

    char pad_spaces[128];
    if (pad > sizeof(pad_spaces) - 1) pad = sizeof(pad_spaces) - 1;
    memset(pad_spaces, ' ', pad);
    pad_spaces[pad] = '\0';

    ESP_LOGI(TAG, "| %s%s |", content, pad_spaces);
}

static void on_pc_quota_updated(const pc_quota_packet_t *quota) {
    // Temporarily muted during LCD diagnostic to keep serial output 100% clean
    display_ui_update_quota(quota);

    // Nếu Quota nguy cấp (<20%), nháy LED đỏ cảnh báo
    if (quota->alert_level == QUOTA_ALERT_CRITICAL && led_strip) {
        led_strip_set_pixel(led_strip, 0, 70, 0, 0); // Đỏ sáng cảnh báo
        led_strip_refresh(led_strip);
    }
}

static void on_ble_raw_data_received(const uint8_t *data, size_t len) {
    pc_sync_process_packet(data, len);
}

esp_err_t launchpad_core_init(void) {
    // 1. Khởi tạo tầng giao tiếp PC Sync với callback xử lý dữ liệu
    pc_sync_init(on_pc_quota_updated);

    // 2. Đăng ký hàm nhận gói tin BLE thô từ ble_hid chuyển sang bộ bóc tách pc_sync
    ble_hid_register_pc_sync_cb(on_ble_raw_data_received);

    // 3. Khởi tạo màn hình TFT 2.8" SPI và engine đồ họa LVGL v8
    display_ui_init();

    xTaskCreate(launchpad_button_task, "lp_btn_task", 3072, NULL, 10, NULL);
    return ESP_OK;
}
