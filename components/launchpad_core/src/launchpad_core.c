#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "ble_hid.h"
#include "pc_sync.h"
#include "display_ui.h"
#include "launchpad_core.h"
#include "analog_joystick.h"
#include "input_config.h"

static const char *TAG = "LP_CORE";

#define LED_STRIP_GPIO       GPIO_NUM_48
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

#define MODE_SWITCH_TICKS    pdMS_TO_TICKS(2000) // Giữ 2 giây: Chuyển Mode
#define BOND_CLEAR_TICKS     pdMS_TO_TICKS(6000) // Giữ 6 giây: Xóa ghép đôi Bluetooth

static launchpad_mode_t current_mode = LP_MODE_GAMEPAD;
static const char *mode_names[LP_MODE_MAX] = {
    "GAMEPAD 🎮 (Xbox Wireless Controller)",
    "KEYBOARD & MOUSE ⌨️🖱️ (Bàn phím + Chuột)"
};

static led_strip_handle_t led_strip = NULL;

// ==============================================================================
// 1. WS2812 RGB LED INDICATION
// ==============================================================================
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
        case LP_MODE_KB_MOUSE:
            // 🟢 GREEN / CYAN = Keyboard & Mouse Mode
            led_strip_set_pixel(led_strip, 0, 0, 35, 20);
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

// ==============================================================================
// 2. INPUT MODE MANAGEMENT
// ==============================================================================
static void launchpad_release_all_inputs(void) {
    if (ble_hid_is_connected()) {
        switch (current_mode) {
            case LP_MODE_GAMEPAD:
                ble_hid_send_xbox_gamepad(32768, 32768, 32768, 32768, 0, 0, 0, GAMEPAD_DPAD_CENTER);
                break;
            case LP_MODE_KB_MOUSE:
                ble_hid_send_keyboard_report(0, NULL, 0);
                ble_hid_send_mouse(0, 0, 0, 0);
                break;
            default:
                break;
        }
    }
}

void launchpad_core_set_mode(launchpad_mode_t mode) {
    if (mode >= LP_MODE_MAX) return;
    launchpad_release_all_inputs();
    current_mode = mode;
    launchpad_set_led_mode(current_mode);
    display_ui_set_active_mode(current_mode);
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "🔀 ĐÃ CHUYỂN SANG MODE: %s", mode_names[current_mode]);
    ESP_LOGI(TAG, "==================================================");
}

launchpad_mode_t launchpad_core_get_mode(void) {
    return current_mode;
}

static void on_ui_mode_change_requested(uint8_t mode) {
    launchpad_core_set_mode((launchpad_mode_t)mode);
}

// Backward compatible pad trigger
void launchpad_core_trigger_pad(uint8_t pad_index, bool pressed) {
    if (!ble_hid_is_connected()) return;
    if (pad_index == 0) {
        if (current_mode == LP_MODE_GAMEPAD) {
            if (pressed) {
                ble_hid_send_xbox_gamepad(32768, 0, 32768, 32768, 1023, 0, XBOX_BUTTON_A, GAMEPAD_DPAD_UP);
            } else {
                ble_hid_send_xbox_gamepad(32768, 32768, 32768, 32768, 0, 0, 0, GAMEPAD_DPAD_CENTER);
            }
        } else {
            if (pressed) {
                ble_hid_send_keyboard(0, HID_KEY_SPACE);
                ble_hid_send_mouse(MOUSE_BUTTON_LEFT, 0, 0, 0);
            } else {
                ble_hid_send_keyboard(0, 0);
                ble_hid_send_mouse(0, 0, 0, 0);
            }
        }
    }
}

// ==============================================================================
// 3. HARDWARE BUTTON INITIALIZATION (WITH INTERNAL PULL-UP)
// ==============================================================================
static void launchpad_hardware_buttons_init(void) {
    uint64_t pin_mask = 0;

    // Gom toàn bộ chân nút từ bảng cấu hình BUTTON_CONFIGS
    for (int i = 0; i < TOTAL_CONFIG_BUTTONS; i++) {
        pin_mask |= (1ULL << BUTTON_CONFIGS[i].gpio);
    }

    // Thêm chân nút BOOT (GPIO 0)
    pin_mask |= (1ULL << PIN_BOOT_BTN);

    // Cấu hình kéo cao nội bộ (Internal Pull-Up) cho TẤT CẢ các chân nút
    gpio_config_t io_conf = {
        .pin_bit_mask = pin_mask,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,     // ⚡ BẬT TRỞ KÉO CAO NỘI (ACTIVE LOW)
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "⚡ Đã bật điện trở kéo cao nội (Pull-up) cho toàn bộ %d nút:", TOTAL_CONFIG_BUTTONS + 1);
        for (int i = 0; i < TOTAL_CONFIG_BUTTONS; i++) {
            ESP_LOGI(TAG, "   • %-6s -> GPIO %2d", BUTTON_CONFIGS[i].name, BUTTON_CONFIGS[i].gpio);
        }
        ESP_LOGI(TAG, "   • BOOT   -> GPIO %2d (Mode Switch / Clear Bonds)", PIN_BOOT_BTN);
    } else {
        ESP_LOGE(TAG, "❌ Lỗi cấu hình GPIO pull-up: %s", esp_err_to_name(ret));
    }
}

// ==============================================================================
// 4. MAIN INPUT POLLING & DEBUG LOGGING TASK
// ==============================================================================
static void launchpad_input_task(void *pvParameters) {
    // Trạng thái nút trước đó (true = đang nhả / HIGH, false = đang nhấn / LOW)
    bool btn_pressed[TOTAL_CONFIG_BUTTONS] = {0};

    // BOOT Button tracking
    bool boot_last_pressed = false;
    TickType_t boot_press_start_time = 0;
    bool boot_mode_switched = false;
    bool boot_bond_cleared = false;

    // Gamepad HID state caching (tránh spam BLE indicate khi không có thay đổi)
    uint16_t last_gamepad_buttons = 0;
    uint16_t last_xbox_x = 32768;
    uint16_t last_xbox_y = 32768;

    // Mouse & Keyboard HID state caching
    uint8_t last_mouse_btns = 0;
    uint8_t last_keys[6] = {0};
    uint8_t last_key_count = 0;
    uint8_t last_mod = 0;

    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "🎮 BẮT ĐẦU TASK QUÉT NÚT NHẤN & JOYSTICK (15ms)");
    ESP_LOGI(TAG, "Mode ban đầu: %s", mode_names[current_mode]);
    ESP_LOGI(TAG, "💡 HƯỚNG DẪN:");
    ESP_LOGI(TAG, "   - Bấm nút bất kỳ để kích hoạt hành động và in debug log.");
    ESP_LOGI(TAG, "   - Giữ nút BOOT (GPIO 0) 2s: Chuyển Mode (🎮 Gamepad <-> ⌨️🖱️ KB/Mouse).");
    ESP_LOGI(TAG, "   - Giữ nút BOOT (GPIO 0) >6s: Xóa bộ nhớ Bluetooth Pairing.");
    ESP_LOGI(TAG, "==================================================");

    while (1) {
        // ----------------------------------------------------------------------
        // 1. Quét nút BOOT (GPIO 0): Đổi Mode (2s) hoặc Xóa Pairing (6s)
        // ----------------------------------------------------------------------
        bool boot_pressed = (gpio_get_level(PIN_BOOT_BTN) == 0);

        if (boot_pressed && !boot_last_pressed) {
            boot_last_pressed = true;
            boot_press_start_time = xTaskGetTickCount();
            boot_mode_switched = false;
            boot_bond_cleared = false;
            ESP_LOGI(TAG, "🔘 [BUTTON PRESSED]  BOOT   (GPIO  0) | Giữ 2s đổi mode, giữ 6s xóa pairing");
        } else if (boot_pressed && boot_last_pressed) {
            TickType_t duration = xTaskGetTickCount() - boot_press_start_time;
            if (!boot_mode_switched && duration >= MODE_SWITCH_TICKS) {
                boot_mode_switched = true;
                launchpad_core_set_mode((current_mode + 1) % LP_MODE_MAX);
            }
            if (!boot_bond_cleared && duration >= BOND_CLEAR_TICKS) {
                boot_bond_cleared = true;
                ESP_LOGW(TAG, "🔄 ĐÃ GIỮ BOOT >6 GIÂY -> XÓA BỘ NHỚ PAIRING ĐỂ GHÉP ĐÔI MỚI!");
                launchpad_flash_led_bond_clear();
                ble_hid_clear_bonds();
            }
        } else if (!boot_pressed && boot_last_pressed) {
            boot_last_pressed = false;
            ESP_LOGI(TAG, "⚪ [BUTTON RELEASED] BOOT   (GPIO  0)");
        }

        // ----------------------------------------------------------------------
        // 2. Quét 7 nút (BTN 1 -> BTN 6 + JOY_SW) & IN DEBUG LOG KHI NHẤN
        // ----------------------------------------------------------------------
        for (int i = 0; i < TOTAL_CONFIG_BUTTONS; i++) {
            const button_config_t *btn = &BUTTON_CONFIGS[i];
            bool is_pressed = (gpio_get_level(btn->gpio) == 0); // Active LOW

            if (is_pressed && !btn_pressed[i]) {
                // Sự kiện: BẮT ĐẦU NHẤN XUỐNG
                btn_pressed[i] = true;

                // IN DEBUG LOG RÕ RÀNG THEO YÊU CẦU CỦA USER:
                const char *action_name = (current_mode == LP_MODE_GAMEPAD) ? btn->gamepad_name : btn->kbm_name;
                ESP_LOGI(TAG, "🔘 [BUTTON PRESSED]  %-6s (GPIO %2d) | Chế độ: %-14s | Chức năng: %s",
                         btn->name, btn->gpio,
                         current_mode == LP_MODE_GAMEPAD ? "GAMEPAD 🎮" : "KB/MOUSE ⌨️🖱️",
                         action_name);

                if (!ble_hid_is_connected()) {
                    ESP_LOGD(TAG, "   (Chưa kết nối BLE Host - hãy ghép đôi với PC để truyền tín hiệu)");
                }

            } else if (!is_pressed && btn_pressed[i]) {
                // Sự kiện: NHẢ NÚT RA
                btn_pressed[i] = false;

                ESP_LOGI(TAG, "⚪ [BUTTON RELEASED] %-6s (GPIO %2d)", btn->name, btn->gpio);
            }
        }

        // ----------------------------------------------------------------------
        // 3. Đọc dữ liệu Analog Joystick (VRx, VRy)
        // ----------------------------------------------------------------------
        joystick_state_t joy;
        analog_joystick_read(&joy);

        // ----------------------------------------------------------------------
        // 4. Xử lý & Gửi báo cáo BLE HID dựa trên Mode hiện tại
        // ----------------------------------------------------------------------
        if (ble_hid_is_connected()) {
            if (current_mode == LP_MODE_GAMEPAD) {
                // === GAMEPAD MODE (Xbox Controller) ===
                uint16_t gp_buttons = 0;
                for (int i = 0; i < TOTAL_CONFIG_BUTTONS; i++) {
                    if (btn_pressed[i]) {
                        gp_buttons |= BUTTON_CONFIGS[i].gamepad_button;
                    }
                }

                // Trục cần gạt Xbox Left Stick (0..65535, giữa 32768)
                uint16_t xbox_x = analog_joystick_to_xbox_axis(joy.x);
                uint16_t xbox_y = analog_joystick_to_xbox_axis(joy.y);

                // Gửi báo cáo khi: nút thay đổi, cần di chuyển, hoặc cần vừa về giữa
                bool btn_changed = (gp_buttons != last_gamepad_buttons);
                bool joy_moved = (abs((int)xbox_x - (int)last_xbox_x) > 300 || abs((int)xbox_y - (int)last_xbox_y) > 300);
                if (btn_changed || joy_moved || (joy.x != 0 || joy.y != 0) || (last_xbox_x != 32768 || last_xbox_y != 32768)) {
                    ble_hid_send_xbox_gamepad(xbox_x, xbox_y, 32768, 32768, 0, 0, gp_buttons, GAMEPAD_DPAD_CENTER);
                    last_gamepad_buttons = gp_buttons;
                    last_xbox_x = xbox_x;
                    last_xbox_y = xbox_y;
                }

            } else if (current_mode == LP_MODE_KB_MOUSE) {
                // === KEYBOARD & MOUSE MODE ===

                // a) Thu thập các phím bàn phím đang nhấn
                uint8_t cur_keys[6] = {0};
                uint8_t cur_key_count = 0;
                uint8_t cur_mod = 0;

                // b) Thu thập các nút chuột và media keys đang nhấn
                uint8_t cur_mouse_btns = 0;
                uint8_t cur_consumer = 0;

                for (int i = 0; i < TOTAL_CONFIG_BUTTONS; i++) {
                    if (btn_pressed[i]) {
                        const button_config_t *btn = &BUTTON_CONFIGS[i];
                        if (btn->kbm_type == ACTION_TYPE_KEYBOARD) {
                            if (cur_key_count < 6 && btn->kbm_code != 0) {
                                cur_keys[cur_key_count++] = btn->kbm_code;
                            }
                            cur_mod |= btn->kbm_modifier;
                        } else if (btn->kbm_type == ACTION_TYPE_MOUSE_BTN) {
                            cur_mouse_btns |= btn->kbm_code;
                        } else if (btn->kbm_type == ACTION_TYPE_CONSUMER) {
                            cur_consumer |= btn->kbm_code;
                        }
                    }
                }

                // Gửi Consumer Control (Media Keys) khi trạng thái thay đổi (nhấn gửi bitmask, nhả gửi 0)
                static uint8_t last_consumer = 0;
                if (cur_consumer != last_consumer) {
                    ble_hid_send_consumer(cur_consumer);
                    last_consumer = cur_consumer;
                }

                // Gửi bàn phím nếu có thay đổi
                bool kb_changed = (cur_key_count != last_key_count) ||
                                  (cur_mod != last_mod) ||
                                  (memcmp(cur_keys, last_keys, sizeof(cur_keys)) != 0);
                if (kb_changed) {
                    ble_hid_send_keyboard_report(cur_mod, cur_keys, cur_key_count);
                    memcpy(last_keys, cur_keys, sizeof(cur_keys));
                    last_key_count = cur_key_count;
                    last_mod = cur_mod;
                }

                // Tính toán di chuyển chuột từ Joystick chuẩn ThinkPad TrackPoint
                int8_t dx = 0, dy = 0;
                analog_joystick_get_mouse_delta(&joy, &dx, &dy);

                // Gửi chuột nếu nút chuột thay đổi hoặc cần gạt đang dịch chuyển
                if (cur_mouse_btns != last_mouse_btns || dx != 0 || dy != 0) {
                    ble_hid_send_mouse(cur_mouse_btns, dx, dy, 0);
                    last_mouse_btns = cur_mouse_btns;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(JOY_POLL_INTERVAL_MS)); // 15ms debounce & scan loop
    }
}

// ==============================================================================
// 5. TELEMETRY & BLE CALLBACKS
// ==============================================================================
static void on_pc_quota_updated(const pc_quota_packet_t *quota) {
    ESP_LOGI(TAG, "📊 Quota Sync: Gemini=%d%% | Claude=%d%% | CPU=%d%% | Prompts=%lu | Time=%s",
             quota->antigrav_quota_percent, quota->claude_5h_percent, quota->cpu_percent,
             (unsigned long)quota->prompt_count, quota->time_vn);
    display_ui_update_quota(quota);

    if (quota->alert_level == QUOTA_ALERT_CRITICAL && led_strip) {
        led_strip_set_pixel(led_strip, 0, 70, 0, 0); // Đỏ cảnh báo
        led_strip_refresh(led_strip);
    }
}

static void on_ble_raw_data_received(const uint8_t *data, size_t len) {
    pc_sync_process_packet(data, len);
}

static void on_ble_connection_changed(bool connected) {
    ESP_LOGI(TAG, "📶 BLE Connection State: %s", connected ? "CONNECTED" : "DISCONNECTED");
    display_ui_set_ble_connected(connected);
}

// ==============================================================================
// 6. LAUNCHPAD CORE INITIALIZATION
// ==============================================================================
esp_err_t launchpad_core_init(void) {
    ESP_LOGI(TAG, "🚀 Khởi động Universal LaunchPad Core...");

    // 1. Khởi tạo RGB LED trên GPIO 48
    launchpad_led_init();
    launchpad_set_led_mode(current_mode);

    // 2. Khởi tạo Analog Joystick (ADC1 GPIO 1, 2 và SW GPIO 42 có pull-up)
    esp_err_t ret = analog_joystick_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "⚠️ Cảnh báo khởi tạo Analog Joystick: %s", esp_err_to_name(ret));
    }

    // Nạp cấu hình từ input_config.h vào driver Joystick & Chuột
    analog_joystick_config_t joy_cfg = {
        .swap_xy = JOY_SWAP_XY,
        .invert_x = JOY_INVERT_X,
        .invert_y = JOY_INVERT_Y,
        .mouse_deadzone = JOY_MOUSE_DEADZONE,
        .mouse_max_speed = JOY_MOUSE_MAX_SPEED,
        .mouse_return_thresh = JOY_MOUSE_RETURN_THRESH,
    };
    analog_joystick_set_config(&joy_cfg);

    // 3. Khởi tạo toàn bộ các nút nhấn vật lý với điện trở kéo lên nội (Pull-up)
    launchpad_hardware_buttons_init();

    // 4. Khởi tạo tầng giao tiếp PC Sync với callback
    pc_sync_init(on_pc_quota_updated);

    // 5. Đăng ký nhận gói tin BLE thô
    ble_hid_register_pc_sync_cb(on_ble_raw_data_received);

    // 6. Khởi tạo màn hình TFT 2.8" SPI và engine đồ họa LVGL v8
    display_ui_init();

    // 7. Đăng ký callback theo dõi kết nối BLE
    ble_hid_register_conn_status_cb(on_ble_connection_changed);

    // 8. Đăng ký callback khi đổi Mode trên màn hình cảm ứng
    display_ui_register_mode_change_cb(on_ui_mode_change_requested);
    display_ui_set_active_mode(current_mode);

    // 9. Tạo FreeRTOS task đọc nút bấm và joystick (Stack 4096 bytes, Priority 10)
    BaseType_t task_ret = xTaskCreate(launchpad_input_task, "lp_input_task", 4096, NULL, 10, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "❌ Không thể tạo task đọc input lp_input_task!");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "✅ LaunchPad Core đã sẵn sàng phục vụ!");
    return ESP_OK;
}
