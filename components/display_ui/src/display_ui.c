/*
 * AI Developer Desk Console & Universal Gateway
 * Display UI Component (Landscape 320x240, Header on LEFT)
 * Features:
 *  - Screen 1: Quota Dashboard (Large Clock, BLE Signal Bars, Big Gemini/Claude Cards, Home Button)
 *  - Screen 2: Home Screen (Giant Clock 40pt, Date/Calendar, Mini Quota Bars, Back to Console Button)
 *  - Hardware Init: 100% verified configuration (MADCTL = 0xA0)
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lvgl.h"

#include "display_ui.h"
#include "touch_xpt2046.h"
#include "earth_wallpaper.h"
#include "ios_fonts.h"
#include "mode_icons.h"
#include "shiba_sprite.h"
#include "bongo_shiba_sprite.h"
#include "ble_hid.h"

static const char *TAG = "DISP_UI";

#define LCD_HOST                SPI2_HOST
#define PIN_NUM_SCLK            GPIO_NUM_10
#define PIN_NUM_MOSI            GPIO_NUM_11
#define PIN_NUM_MISO            GPIO_NUM_46
#define PIN_NUM_DC              GPIO_NUM_12
#define PIN_NUM_RST             GPIO_NUM_13
#define PIN_NUM_CS              GPIO_NUM_14
#define PIN_NUM_BL              GPIO_NUM_9

#define LCD_H_RES               320
#define LCD_V_RES               240

#define LVGL_TICK_PERIOD_MS     2
#define LVGL_BUFFER_LINES       40

static spi_device_handle_t spi_dev = NULL;
static SemaphoreHandle_t lvgl_mux = NULL;
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t disp_buf;
static lv_disp_t *disp_handle = NULL;

// 5 Screen Handles
static lv_obj_t *scr_console = NULL;
static lv_obj_t *scr_home = NULL;
static lv_obj_t *scr_gamepad = NULL;
static lv_obj_t *scr_kb_mouse = NULL;
static lv_obj_t *scr_full_kb = NULL;
static lv_obj_t *ta_full_kb = NULL;

// Screen 1 (Console) Widget Pointers
static lv_obj_t *lbl_clock = NULL;
static lv_obj_t *arc_gemini = NULL;
static lv_obj_t *lbl_gemini_val = NULL;
static lv_obj_t *lbl_gem_sub = NULL;
static lv_obj_t *bar_gemini_w = NULL;

static lv_obj_t *arc_claude = NULL;
static lv_obj_t *lbl_claude_val = NULL;
static lv_obj_t *lbl_cld_sub = NULL;
static lv_obj_t *bar_claude_w = NULL;

static lv_obj_t *lbl_system = NULL;
static lv_obj_t *btn_paw = NULL;
static lv_obj_t *img_paw = NULL;
static lv_obj_t *img_shiba = NULL;

// Shiba Pet Animation & NVS Persistence State
static bool s_pet_enabled = true;
static int s_shiba_x = 80;
static int s_shiba_dir = 1;       // 1 = moving right, -1 = moving left
static int s_shiba_frame = 0;     // 0..3
static int s_shiba_pause = 0;     // Idle pause countdown ticks
static int s_shiba_hop_ticks = 0; // Happy hop animation ticks when tapped

// Bongo Shiba Animation (Screen 2 Home: 3s gõ phím + 2s nghỉ ngơi)
static lv_obj_t *img_bongo_shiba = NULL;
static uint8_t s_bongo_tick = 0;

// Signal Bars (Screen 1 & Screen 2 & Screen 3A & Screen 3B & Screen 3C)
static lv_obj_t *bar_sig_1 = NULL;
static lv_obj_t *bar_sig_2 = NULL;
static lv_obj_t *bar_sig_3 = NULL;
static lv_obj_t *bar_sig_4 = NULL;
static lv_obj_t *lbl_sig_x = NULL;

static lv_obj_t *h_bar_sig_1 = NULL;
static lv_obj_t *h_bar_sig_2 = NULL;
static lv_obj_t *h_bar_sig_3 = NULL;
static lv_obj_t *h_bar_sig_4 = NULL;
static lv_obj_t *h_lbl_sig_x = NULL;

static lv_obj_t *gp_bar_sig_1 = NULL;
static lv_obj_t *gp_bar_sig_2 = NULL;
static lv_obj_t *gp_bar_sig_3 = NULL;
static lv_obj_t *gp_bar_sig_4 = NULL;
static lv_obj_t *gp_lbl_sig_x = NULL;

static lv_obj_t *km_bar_sig_1 = NULL;
static lv_obj_t *km_bar_sig_2 = NULL;
static lv_obj_t *km_bar_sig_3 = NULL;
static lv_obj_t *km_bar_sig_4 = NULL;
static lv_obj_t *km_lbl_sig_x = NULL;

static lv_obj_t *fkb_bar_sig_1 = NULL;
static lv_obj_t *fkb_bar_sig_2 = NULL;
static lv_obj_t *fkb_bar_sig_3 = NULL;
static lv_obj_t *fkb_bar_sig_4 = NULL;
static lv_obj_t *fkb_lbl_sig_x = NULL;

// Screen 2 (Home) Widget Pointers
static lv_obj_t *lbl_home_big_clock = NULL;
static lv_obj_t *lbl_home_calendar = NULL;
static lv_obj_t *bar_home_gemini = NULL;
static lv_obj_t *lbl_home_gem_val = NULL;
static lv_obj_t *bar_home_claude = NULL;
static lv_obj_t *lbl_home_cld_val = NULL;

// Mode Selection Widgets on Home Screen
static lv_obj_t *btn_home_gp = NULL;
static lv_obj_t *btn_home_km = NULL;
static lv_obj_t *img_home_gp = NULL;
static lv_obj_t *img_home_km = NULL;

// Mode Callback & State
static display_ui_mode_change_cb_t s_mode_change_cb = NULL;
static uint8_t s_active_mode = 0; // 0 = Gamepad, 1 = Keyboard+Mouse

// Autonomous Internal Timekeeping & BLE State
static bool s_ble_connected = false;
static bool s_clock_synced = false;
static uint32_t s_last_packet_time_ms = 0;
static int32_t s_gemini_reset_sec = -1;
static int32_t s_claude_reset_sec = -1;
static uint8_t s_gemini_quota_pct = 100;
static uint8_t s_claude_quota_pct = 100;
static char s_last_rendered_hm[8] = "";
static int s_current_signal_level = -1;

esp_err_t display_ui_set_backlight(uint8_t brightness_pct) {
    gpio_set_level(PIN_NUM_BL, brightness_pct > 0 ? 1 : 0);
    return ESP_OK;
}

static void send_cmd(uint8_t cmd) {
    gpio_set_level(PIN_NUM_DC, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(spi_dev, &t);
}

static void send_data(const uint8_t *data, size_t len) {
    if (len == 0) return;
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_polling_transmit(spi_dev, &t);
}

static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    send_cmd(0x2A); // CASET
    uint8_t caset[] = { (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF), (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF) };
    send_data(caset, 4);

    send_cmd(0x2B); // RASET
    uint8_t raset[] = { (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF), (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF) };
    send_data(raset, 4);

    send_cmd(0x2C); // RAMWR
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    uint16_t x0 = area->x1;
    uint16_t y0 = area->y1;
    uint16_t x1 = area->x2;
    uint16_t y1 = area->y2;

    set_window(x0, y0, x1, y1);

    size_t pixel_count = (x1 - x0 + 1) * (y1 - y0 + 1);
    gpio_set_level(PIN_NUM_DC, 1);

    spi_transaction_t t = {
        .length = pixel_count * 16,
        .tx_buffer = color_map,
    };
    spi_device_polling_transmit(spi_dev, &t);

    lv_disp_flush_ready(drv);
}

static void lvgl_tick_cb(void *arg) {
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static bool lvgl_lock(int timeout_ms) {
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

static void lvgl_unlock(void) {
    xSemaphoreGiveRecursive(lvgl_mux);
}

static void lvgl_task(void *arg) {
    ESP_LOGI(TAG, "LVGL task running");
    while (1) {
        if (lvgl_lock(-1)) {
            uint32_t delay_ms = lv_timer_handler();
            lvgl_unlock();
            if (delay_ms > 50) delay_ms = 50;
            if (delay_ms < 5) delay_ms = 5;
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

// Helper: Create 4-bar cellular signal widget
static lv_obj_t* create_signal_widget(lv_obj_t *parent, lv_obj_t **b1, lv_obj_t **b2, lv_obj_t **b3, lv_obj_t **b4, lv_obj_t **x_lbl) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 26, 18);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    int heights[] = {4, 7, 10, 14};
    int xs[] = {1, 6, 11, 16};
    lv_obj_t **bars[] = {b1, b2, b3, b4};

    for (int i = 0; i < 4; i++) {
        lv_obj_t *bar = lv_obj_create(cont);
        lv_obj_set_pos(bar, xs[i], 16 - heights[i]);
        lv_obj_set_size(bar, 3, heights[i]);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x475569), 0); // Default grey (disconnected)
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bar, 1, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
        *(bars[i]) = bar;
    }

    lv_obj_t *lbl_x = lv_label_create(cont);
    lv_label_set_text(lbl_x, "x");
    lv_obj_set_style_text_color(lbl_x, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_text_font(lbl_x, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_x, LV_ALIGN_CENTER, 1, -1);
    *x_lbl = lbl_x;

    return cont;
}

// Update signal bars
static void set_signal_level(int level) {
    if (s_current_signal_level == level) return;
    s_current_signal_level = level;

    lv_color_t c_on = lv_palette_main(LV_PALETTE_GREEN);
    lv_color_t c_off = lv_color_hex(0x475569);

    lv_obj_t *b1_arr[] = {bar_sig_1, h_bar_sig_1, gp_bar_sig_1, km_bar_sig_1, fkb_bar_sig_1};
    lv_obj_t *b2_arr[] = {bar_sig_2, h_bar_sig_2, gp_bar_sig_2, km_bar_sig_2, fkb_bar_sig_2};
    lv_obj_t *b3_arr[] = {bar_sig_3, h_bar_sig_3, gp_bar_sig_3, km_bar_sig_3, fkb_bar_sig_3};
    lv_obj_t *b4_arr[] = {bar_sig_4, h_bar_sig_4, gp_bar_sig_4, km_bar_sig_4, fkb_bar_sig_4};
    lv_obj_t *x_arr[]  = {lbl_sig_x, h_lbl_sig_x, gp_lbl_sig_x, km_lbl_sig_x, fkb_lbl_sig_x};

    for (int s = 0; s < 5; s++) {
        if (!b1_arr[s]) continue;

        if (level <= 0) {
            lv_obj_set_style_bg_color(b1_arr[s], c_off, 0);
            lv_obj_set_style_bg_color(b2_arr[s], c_off, 0);
            lv_obj_set_style_bg_color(b3_arr[s], c_off, 0);
            lv_obj_set_style_bg_color(b4_arr[s], c_off, 0);
            if (x_arr[s]) lv_obj_clear_flag(x_arr[s], LV_OBJ_FLAG_HIDDEN);
        } else {
            if (x_arr[s]) lv_obj_add_flag(x_arr[s], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(b1_arr[s], level >= 1 ? c_on : c_off, 0);
            lv_obj_set_style_bg_color(b2_arr[s], level >= 2 ? c_on : c_off, 0);
            lv_obj_set_style_bg_color(b3_arr[s], level >= 3 ? c_on : c_off, 0);
            lv_obj_set_style_bg_color(b4_arr[s], level >= 4 ? c_on : c_off, 0);
        }
    }
}

// Cập nhật trạng thái BLE ngay tức thì (từ callback ngắt kết nối BLE)
esp_err_t display_ui_set_ble_connected(bool connected) {
    s_ble_connected = connected;
    if (!connected) {
        s_last_packet_time_ms = 0;
    }
    if (lvgl_lock(100)) {
        set_signal_level(connected ? 4 : 0);
        lvgl_unlock();
    }
    return ESP_OK;
}

// Timer định kỳ 1 giây: Tự động đếm giây và cập nhật đồng hồ trên màn hình
static void clock_tick_timer_cb(lv_timer_t *timer) {
    // 1. Kiểm tra timeout nhận dữ liệu BLE (nếu mất BLE hoặc quá 25s không có gói tin thì báo disconnect)
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    if (!s_ble_connected || (s_last_packet_time_ms > 0 && (now_ms - s_last_packet_time_ms) > 25000)) {
        set_signal_level(0);
    }

    // 2. Nếu đã đồng bộ thời gian từ PC ít nhất 1 lần, ESP32 tự động tính toán và cập nhật giao diện
    if (s_clock_synced) {
        time_t now = time(NULL);
        struct tm ti;
        localtime_r(&now, &ti);

        // Header trên Dashboard Console: HH:MM:SS
        char time_hms[16];
        snprintf(time_hms, sizeof(time_hms), "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);
        if (lbl_clock) {
            lv_label_set_text(lbl_clock, time_hms);
        }

        // Màn hình Home: Đồng hồ số to HH:MM (chỉ cập nhật khi đổi phút để tiết kiệm CPU & tránh redraw lớn)
        char time_hm[8];
        snprintf(time_hm, sizeof(time_hm), "%02d:%02d", ti.tm_hour, ti.tm_min);
        if (strcmp(s_last_rendered_hm, time_hm) != 0) {
            strncpy(s_last_rendered_hm, time_hm, sizeof(s_last_rendered_hm) - 1);
            if (lbl_home_big_clock) {
                lv_label_set_text(lbl_home_big_clock, time_hm);
            }
        }

        // Màn hình Home: Cập nhật thứ ngày tháng
        static const char *weekday_vn_map[7] = {
            "CN", "Th 2", "Th 3", "Th 4", "Th 5", "Th 6", "Th 7"
        };
        char date_buf[32];
        snprintf(date_buf, sizeof(date_buf), "%s,  %d thg %d",
                 weekday_vn_map[ti.tm_wday % 7], ti.tm_mday, ti.tm_mon + 1);
        if (lbl_home_calendar) {
            lv_label_set_text(lbl_home_calendar, date_buf);
        }

        // 3. Tự động đếm lùi thời gian reset Quota 5h (từng giây định dạng HH:MM:SS)
        if (s_gemini_reset_sec > 0) {
            s_gemini_reset_sec--;
            char buf[16];
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                     (int)(s_gemini_reset_sec / 3600),
                     (int)((s_gemini_reset_sec % 3600) / 60),
                     (int)(s_gemini_reset_sec % 60));
            if (lbl_gem_sub) lv_label_set_text(lbl_gem_sub, buf);
        } else if (s_gemini_reset_sec == 0) {
            if (lbl_gem_sub) lv_label_set_text(lbl_gem_sub, "00:00:00");
        }

        if (s_claude_reset_sec > 0) {
            s_claude_reset_sec--;
            char buf[16];
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                     (int)(s_claude_reset_sec / 3600),
                     (int)((s_claude_reset_sec % 3600) / 60),
                     (int)(s_claude_reset_sec % 60));
            if (lbl_cld_sub) lv_label_set_text(lbl_cld_sub, buf);
        } else if (s_claude_reset_sec == 0) {
            if (lbl_cld_sub) lv_label_set_text(lbl_cld_sub, "00:00:00");
        }
    }
}

// Đăng ký callback khi chạm nút đổi Mode
void display_ui_register_mode_change_cb(display_ui_mode_change_cb_t cb) {
    s_mode_change_cb = cb;
}

// Cập nhật giao diện nút Mode đang kích hoạt (Highlight viền và màu)
esp_err_t display_ui_set_active_mode(uint8_t mode) {
    s_active_mode = mode;
    if (lvgl_lock(100)) {
        if (btn_home_gp && btn_home_km) {
            if (mode == 0) { // GAMEPAD ACTIVE
                lv_obj_set_style_border_color(btn_home_gp, lv_color_hex(0x38BDF8), 0); // Neon Cyan
                lv_obj_set_style_border_width(btn_home_gp, 2, 0);
                lv_obj_set_style_bg_color(btn_home_gp, lv_color_hex(0x1E293B), 0);
                lv_obj_set_style_bg_opa(btn_home_gp, LV_OPA_90, 0);

                lv_obj_set_style_border_color(btn_home_km, lv_color_hex(0x334155), 0);
                lv_obj_set_style_border_width(btn_home_km, 1, 0);
                lv_obj_set_style_bg_color(btn_home_km, lv_color_hex(0x0F172A), 0);
                lv_obj_set_style_bg_opa(btn_home_km, LV_OPA_60, 0);
            } else { // KEYBOARD + MOUSE ACTIVE
                lv_obj_set_style_border_color(btn_home_km, lv_color_hex(0x10B981), 0); // Emerald Green
                lv_obj_set_style_border_width(btn_home_km, 2, 0);
                lv_obj_set_style_bg_color(btn_home_km, lv_color_hex(0x1E293B), 0);
                lv_obj_set_style_bg_opa(btn_home_km, LV_OPA_90, 0);

                lv_obj_set_style_border_color(btn_home_gp, lv_color_hex(0x334155), 0);
                lv_obj_set_style_border_width(btn_home_gp, 1, 0);
                lv_obj_set_style_bg_color(btn_home_gp, lv_color_hex(0x0F172A), 0);
                lv_obj_set_style_bg_opa(btn_home_gp, LV_OPA_60, 0);
            }
        }
        lvgl_unlock();
    }
    return ESP_OK;
}

// Navigation event callbacks
static void nav_to_home_cb(lv_event_t *e) {
    if (scr_home) {
        lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
    }
}

static void nav_to_console_cb(lv_event_t *e) {
    if (scr_console) {
        lv_scr_load_anim(scr_console, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 250, 0, false);
    }
}

static void nav_to_gamepad_cb(lv_event_t *e) {
    display_ui_set_active_mode(0);
    if (s_mode_change_cb) {
        s_mode_change_cb(0);
    }
    if (scr_gamepad) {
        lv_scr_load_anim(scr_gamepad, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
    }
}

static void nav_to_kb_mouse_cb(lv_event_t *e) {
    display_ui_set_active_mode(1);
    if (s_mode_change_cb) {
        s_mode_change_cb(1);
    }
    if (scr_kb_mouse) {
        lv_scr_load_anim(scr_kb_mouse, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
    }
}

static void nav_back_to_home_cb(lv_event_t *e) {
    if (scr_home) {
        lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 250, 0, false);
    }
}

// Gamepad virtual controls state
static uint16_t s_gp_buttons = 0;
static uint8_t  s_gp_dpad = GAMEPAD_DPAD_CENTER;
static uint16_t s_gp_l2 = 0;
static uint16_t s_gp_r2 = 0;

static void send_current_gamepad_state(void) {
    if (ble_hid_is_connected()) {
        ble_hid_send_xbox_gamepad(32768, 32768, 32768, 32768, s_gp_l2, s_gp_r2, s_gp_buttons, s_gp_dpad);
    }
}

static void gp_btn_action_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t btn_mask = (uint32_t)(uintptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED) {
        s_gp_buttons |= (uint16_t)btn_mask;
        send_current_gamepad_state();
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        s_gp_buttons &= ~((uint16_t)btn_mask);
        send_current_gamepad_state();
    }
}

static void gp_dpad_action_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint8_t dpad_dir = (uint8_t)(uintptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED) {
        s_gp_dpad = dpad_dir;
        send_current_gamepad_state();
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        s_gp_dpad = GAMEPAD_DPAD_CENTER;
        send_current_gamepad_state();
    }
}

static void gp_trigger_action_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t which = (uint32_t)(uintptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED) {
        if (which == 1) s_gp_l2 = 1023;
        else if (which == 2) s_gp_r2 = 1023;
        send_current_gamepad_state();
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (which == 1) s_gp_l2 = 0;
        else if (which == 2) s_gp_r2 = 0;
        send_current_gamepad_state();
    }
}

// Mouse & Keyboard virtual controls state
static void mouse_btn_action_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint8_t btn = (uint8_t)(uintptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED) {
        ble_hid_send_mouse(btn, 0, 0, 0);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        ble_hid_send_mouse(0, 0, 0, 0);
    }
}

static void mouse_scroll_action_cb(lv_event_t *e) {
    int8_t wheel = (int8_t)(intptr_t)lv_event_get_user_data(e);
    ble_hid_send_mouse(0, 0, 0, wheel);
}

static lv_point_t s_trackpad_last_pt = {0, 0};
static void trackpad_touch_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;

    if (code == LV_EVENT_PRESSED) {
        lv_indev_get_point(indev, &s_trackpad_last_pt);
    } else if (code == LV_EVENT_PRESSING) {
        lv_point_t pt;
        lv_indev_get_point(indev, &pt);
        int dx = pt.x - s_trackpad_last_pt.x;
        int dy = pt.y - s_trackpad_last_pt.y;
        if (dx != 0 || dy != 0) {
            ble_hid_send_mouse(0, (int8_t)(dx * 2), (int8_t)(dy * 2), 0);
            s_trackpad_last_pt = pt;
        }
    }
}

static void kb_key_action_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t param = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    uint8_t modifier = (uint8_t)(param >> 8);
    uint8_t key_code = (uint8_t)(param & 0xFF);

    if (code == LV_EVENT_PRESSED) {
        ble_hid_send_keyboard(modifier, key_code);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        ble_hid_send_keyboard(0, 0);
    }
}

static void media_key_action_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint8_t mask = (uint8_t)(uintptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED) {
        ble_hid_send_consumer(mask);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        ble_hid_send_consumer(0);
    }
}

// Loa bật/tắt (Toggle Mute)
static bool s_is_muted = false;
static lv_obj_t *lbl_mute_icon = NULL;

static void mute_btn_action_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        s_is_muted = !s_is_muted;
        if (lbl_mute_icon) {
            lv_label_set_text(lbl_mute_icon, s_is_muted ? LV_SYMBOL_MUTE : LV_SYMBOL_VOLUME_MAX);
            lv_obj_set_style_text_color(lbl_mute_icon, s_is_muted ? lv_color_hex(0xEF4444) : lv_color_hex(0x10B981), 0);
        }
        ble_hid_send_consumer(CONSUMER_MUTE);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        ble_hid_send_consumer(0);
    }
}

// Chuyển ký tự ASCII sang HID scan code và gửi qua BLE
static void send_hid_ascii(char c) {
    uint8_t mod = 0;
    uint8_t code = 0;
    if (c >= 'a' && c <= 'z') {
        code = 0x04 + (c - 'a');
    } else if (c >= 'A' && c <= 'Z') {
        mod = KEY_MOD_LSHIFT;
        code = 0x04 + (c - 'A');
    } else if (c >= '1' && c <= '9') {
        code = 0x1E + (c - '1');
    } else if (c == '0') {
        code = 0x27;
    } else if (c == ' ') {
        code = HID_KEY_SPACE;
    } else if (c == '\n' || c == '\r') {
        code = HID_KEY_ENTER;
    } else {
        switch (c) {
            case '!': mod = KEY_MOD_LSHIFT; code = 0x1E; break;
            case '@': mod = KEY_MOD_LSHIFT; code = 0x1F; break;
            case '#': mod = KEY_MOD_LSHIFT; code = 0x20; break;
            case '$': mod = KEY_MOD_LSHIFT; code = 0x21; break;
            case '%': mod = KEY_MOD_LSHIFT; code = 0x22; break;
            case '^': mod = KEY_MOD_LSHIFT; code = 0x23; break;
            case '&': mod = KEY_MOD_LSHIFT; code = 0x24; break;
            case '*': mod = KEY_MOD_LSHIFT; code = 0x25; break;
            case '(': mod = KEY_MOD_LSHIFT; code = 0x26; break;
            case ')': mod = KEY_MOD_LSHIFT; code = 0x27; break;
            case '-': code = 0x2D; break;
            case '_': mod = KEY_MOD_LSHIFT; code = 0x2D; break;
            case '=': code = 0x2E; break;
            case '+': mod = KEY_MOD_LSHIFT; code = 0x2E; break;
            case '[': code = 0x2F; break;
            case '{': mod = KEY_MOD_LSHIFT; code = 0x2F; break;
            case ']': code = 0x30; break;
            case '}': mod = KEY_MOD_LSHIFT; code = 0x30; break;
            case '\\': code = 0x31; break;
            case '|': mod = KEY_MOD_LSHIFT; code = 0x31; break;
            case ';': code = 0x33; break;
            case ':': mod = KEY_MOD_LSHIFT; code = 0x33; break;
            case '\'': code = 0x34; break;
            case '\"': mod = KEY_MOD_LSHIFT; code = 0x34; break;
            case '`': code = 0x35; break;
            case '~': mod = KEY_MOD_LSHIFT; code = 0x35; break;
            case ',': code = 0x36; break;
            case '<': mod = KEY_MOD_LSHIFT; code = 0x36; break;
            case '.': code = 0x37; break;
            case '>': mod = KEY_MOD_LSHIFT; code = 0x37; break;
            case '/': code = 0x38; break;
            case '?': mod = KEY_MOD_LSHIFT; code = 0x38; break;
            default: break;
        }
    }
    if (code != 0) {
        ble_hid_send_keyboard(mod, code);
        vTaskDelay(pdMS_TO_TICKS(15));
        ble_hid_send_keyboard(0, 0);
    }
}

static void nav_to_full_kb_cb(lv_event_t *e) {
    if (scr_full_kb) {
        lv_scr_load_anim(scr_full_kb, LV_SCR_LOAD_ANIM_MOVE_TOP, 200, 0, false);
    }
}

static void nav_back_to_kb_mouse_cb(lv_event_t *e) {
    if (scr_kb_mouse) {
        lv_scr_load_anim(scr_kb_mouse, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 200, 0, false);
    }
}

static void __attribute__((unused)) clr_ta_action_cb(lv_event_t *e) {
    if (ta_full_kb) {
        lv_textarea_set_text(ta_full_kb, "");
    }
}

static void full_kb_action_cb(lv_event_t *e) {
    lv_obj_t *kb = lv_event_get_target(e);
    uint16_t btn_id = lv_btnmatrix_get_selected_btn(kb);
    if (btn_id == LV_BTNMATRIX_BTN_NONE) return;

    const char *txt = lv_btnmatrix_get_btn_text(kb, btn_id);
    if (!txt) return;

    if (strcmp(txt, LV_SYMBOL_CLOSE) == 0 || strcmp(txt, LV_SYMBOL_KEYBOARD) == 0) {
        if (scr_kb_mouse) {
            lv_scr_load_anim(scr_kb_mouse, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 200, 0, false);
        }
        return;
    }

    if (strcmp(txt, LV_SYMBOL_OK) == 0 || strcmp(txt, "Enter") == 0 || strcmp(txt, LV_SYMBOL_NEW_LINE) == 0) {
        ble_hid_send_keyboard(0, HID_KEY_ENTER);
        vTaskDelay(pdMS_TO_TICKS(15));
        ble_hid_send_keyboard(0, 0);
        return;
    }

    if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
        ble_hid_send_keyboard(0, HID_KEY_BACKSPACE);
        vTaskDelay(pdMS_TO_TICKS(15));
        ble_hid_send_keyboard(0, 0);
        return;
    }

    if (strcmp(txt, LV_SYMBOL_LEFT) == 0) {
        ble_hid_send_keyboard(0, 0x50); // Arrow Left
        vTaskDelay(pdMS_TO_TICKS(15));
        ble_hid_send_keyboard(0, 0);
        return;
    }

    if (strcmp(txt, LV_SYMBOL_RIGHT) == 0) {
        ble_hid_send_keyboard(0, 0x4F); // Arrow Right
        vTaskDelay(pdMS_TO_TICKS(15));
        ble_hid_send_keyboard(0, 0);
        return;
    }

    if (strcmp(txt, "ABC") == 0 || strcmp(txt, "1#") == 0 || strcmp(txt, "abc") == 0 || strcmp(txt, "+/-") == 0) {
        return;
    }

    if (strlen(txt) == 1) {
        send_hid_ascii(txt[0]);
    }
}

esp_err_t display_ui_toggle_screen(void) {
    if (lvgl_lock(100)) {
        lv_obj_t *act = lv_scr_act();
        if (act == scr_console && scr_home) {
            lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
        } else if (act == scr_home && scr_console) {
            lv_scr_load_anim(scr_console, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 250, 0, false);
        } else if ((act == scr_gamepad || act == scr_kb_mouse || act == scr_full_kb) && scr_home) {
            lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 250, 0, false);
        }
        lvgl_unlock();
    }
    return ESP_OK;
}

// Helper: Create stylized control button
static lv_obj_t* create_control_btn(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                                   const char *text, lv_color_t bg_col, lv_color_t txt_col,
                                   lv_event_cb_t cb, void *user_data, lv_event_code_t filter) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, bg_col, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x475569), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, txt_col, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(btn, cb, filter, user_data);
    return btn;
}

// =========================================================================
// Shiba Pet & Theme Switcher (NVS Persistence & Animation)
// =========================================================================
static void load_pet_setting(void) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("desk_cfg", NVS_READONLY, &my_handle);
    if (err == ESP_OK) {
        uint8_t val = 1;
        if (nvs_get_u8(my_handle, "pet_en", &val) == ESP_OK) {
            s_pet_enabled = (val != 0);
        }
        nvs_close(my_handle);
    }
}

static void save_pet_setting(bool enabled) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("desk_cfg", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        nvs_set_u8(my_handle, "pet_en", enabled ? 1 : 0);
        nvs_commit(my_handle);
        nvs_close(my_handle);
    }
}

static void update_paw_button_state(void) {
    if (!btn_paw || !img_paw) return;
    if (s_pet_enabled) {
        lv_obj_set_style_bg_color(btn_paw, lv_color_hex(0x1E3A5F), 0);
        lv_obj_set_style_border_color(btn_paw, lv_color_hex(0x38BDF8), 0);
        lv_obj_set_style_border_width(btn_paw, 1, 0);
        lv_obj_set_style_img_recolor_opa(img_paw, LV_OPA_TRANSP, 0);
    } else {
        lv_obj_set_style_bg_color(btn_paw, lv_color_hex(0x1E293B), 0);
        lv_obj_set_style_border_color(btn_paw, lv_color_hex(0x334155), 0);
        lv_obj_set_style_border_width(btn_paw, 1, 0);
        lv_obj_set_style_img_recolor(img_paw, lv_color_hex(0x64748B), 0);
        lv_obj_set_style_img_recolor_opa(img_paw, LV_OPA_COVER, 0);
    }
}

static void btn_paw_cb(lv_event_t *e) {
    s_pet_enabled = !s_pet_enabled;
    save_pet_setting(s_pet_enabled);
    update_paw_button_state();
    if (img_shiba) {
        if (s_pet_enabled) {
            lv_obj_clear_flag(img_shiba, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(img_shiba, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void shiba_click_cb(lv_event_t *e) {
    s_shiba_hop_ticks = 8;
    s_shiba_pause = 0;
}

static void shiba_anim_timer_cb(lv_timer_t *timer) {
    if (!s_pet_enabled || !img_shiba || lv_scr_act() != scr_console) {
        return;
    }

    int y_pos = 191;

    if (s_shiba_hop_ticks > 0) {
        s_shiba_hop_ticks--;
        if (s_shiba_hop_ticks % 2 == 1) {
            y_pos = 188; // Nhảy lên 3px
        }
        s_shiba_frame = (s_shiba_frame + 1) % 4;
    } else if (s_shiba_pause > 0) {
        s_shiba_pause--;
        if (s_shiba_pause % 4 == 0) {
            s_shiba_frame = (s_shiba_frame == 0) ? 1 : 0;
        }
    } else {
        s_shiba_frame = (s_shiba_frame + 1) % 4;
        s_shiba_x += s_shiba_dir * 2;

        const int SHIBA_MIN_X = 60;
        const int SHIBA_MAX_X = 132;

        if (s_shiba_x >= SHIBA_MAX_X) {
            s_shiba_x = SHIBA_MAX_X;
            s_shiba_dir = -1;
            s_shiba_pause = 18; // Tạm dừng ~2s
            s_shiba_frame = 0;
        } else if (s_shiba_x <= SHIBA_MIN_X) {
            s_shiba_x = SHIBA_MIN_X;
            s_shiba_dir = 1;
            s_shiba_pause = 18; // Tạm dừng ~2s
            s_shiba_frame = 0;
        }
    }

    const lv_img_dsc_t *sprite = (s_shiba_dir >= 0) 
                                 ? shiba_anim_right[s_shiba_frame] 
                                 : shiba_anim_left[s_shiba_frame];

    lv_img_set_src(img_shiba, sprite);
    lv_obj_set_pos(img_shiba, s_shiba_x, y_pos);
}

static void bongo_shiba_anim_timer_cb(lv_timer_t *timer) {
    if (!img_bongo_shiba || lv_scr_act() != scr_home) {
        return;
    }

    // Chu kỳ 5040ms: 3000ms gõ phím (ticks 0..24, mỗi tick 120ms) + 2040ms nghỉ ngơi (ticks 25..41)
    s_bongo_tick++;
    if (s_bongo_tick >= 42) {
        s_bongo_tick = 0;
    }

    if (s_bongo_tick < 25) {
        // Giai đoạn 3s gõ phím liên tục: Chân trái và chân phải so le đập cực nhanh
        if (s_bongo_tick % 2 == 0) {
            lv_img_set_src(img_bongo_shiba, &img_bongo_tap_l);
        } else {
            lv_img_set_src(img_bongo_shiba, &img_bongo_tap_r);
        }
    } else {
        // Giai đoạn 2s nghỉ ngơi: 2 chân đặt yên trên thanh Quota, mắt cười tít ^_^
        lv_img_set_src(img_bongo_shiba, &img_bongo_rest);
    }
}

// =========================================================================
// SCREEN 1: Quota Dashboard Console
// =========================================================================
static void build_console_screen(void) {
    scr_console = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_console, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(scr_console, LV_OPA_COVER, 0);

    // 1. TOP BAR (Height: 32px)
    lv_obj_t *top_bar = lv_obj_create(scr_console);
    lv_obj_set_pos(top_bar, 0, 0);
    lv_obj_set_size(top_bar, 320, 32);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Large Clock (Montserrat 20)
    lbl_clock = lv_label_create(top_bar);
    lv_label_set_text(lbl_clock, "--:--:--");
    lv_obj_set_style_text_color(lbl_clock, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_clock, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_clock, LV_ALIGN_LEFT_MID, 12, 0);

    // Paw Toggle Button [🐾]
    btn_paw = lv_btn_create(top_bar);
    lv_obj_set_size(btn_paw, 32, 24);
    lv_obj_align(btn_paw, LV_ALIGN_RIGHT_MID, -44, 0);
    lv_obj_set_style_radius(btn_paw, 6, 0);
    lv_obj_set_style_pad_all(btn_paw, 0, 0);
    lv_obj_set_ext_click_area(btn_paw, 6);
    lv_obj_add_event_cb(btn_paw, btn_paw_cb, LV_EVENT_CLICKED, NULL);

    img_paw = lv_img_create(btn_paw);
    lv_img_set_src(img_paw, &img_icon_paw);
    lv_obj_center(img_paw);
    lv_obj_clear_flag(img_paw, LV_OBJ_FLAG_CLICKABLE);

    update_paw_button_state();

    // BLE Signal Bars (Replaces text indicator)
    lv_obj_t *sig_cont = create_signal_widget(top_bar, &bar_sig_1, &bar_sig_2, &bar_sig_3, &bar_sig_4, &lbl_sig_x);
    lv_obj_align(sig_cont, LV_ALIGN_RIGHT_MID, -10, 0);

    // 2. MIDDLE AREA: 2 CARDS (Y = 36, Height = 156)
    // Card Gemini (Left)
    lv_obj_t *card_gemini = lv_obj_create(scr_console);
    lv_obj_set_pos(card_gemini, 8, 36);
    lv_obj_set_size(card_gemini, 148, 156);
    lv_obj_set_style_bg_color(card_gemini, lv_color_hex(0x162032), 0);
    lv_obj_set_style_bg_opa(card_gemini, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card_gemini, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(card_gemini, 1, 0);
    lv_obj_set_style_radius(card_gemini, 10, 0);
    lv_obj_set_style_pad_all(card_gemini, 0, 0);
    lv_obj_clear_flag(card_gemini, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_gem_title = lv_label_create(card_gemini);
    lv_label_set_text(lbl_gem_title, "GEMINI");
    lv_obj_set_style_text_color(lbl_gem_title, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_gem_title, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_gem_title, LV_ALIGN_TOP_MID, 0, 6);

    arc_gemini = lv_arc_create(card_gemini);
    lv_obj_set_size(arc_gemini, 88, 88);
    lv_arc_set_rotation(arc_gemini, 135);
    lv_arc_set_bg_angles(arc_gemini, 0, 270);
    lv_arc_set_value(arc_gemini, 90);
    lv_obj_set_style_arc_color(arc_gemini, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_gemini, 7, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_gemini, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_gemini, 7, LV_PART_INDICATOR);
    lv_obj_remove_style(arc_gemini, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_gemini, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(arc_gemini, LV_ALIGN_TOP_MID, 0, 26);

    lbl_gemini_val = lv_label_create(arc_gemini);
    lv_label_set_text(lbl_gemini_val, "90%");
    lv_obj_set_style_text_color(lbl_gemini_val, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_gemini_val, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_gemini_val);

    // Thời gian đếm ngược reset quota 5h Gemini (thay thế chữ 5h Limit)
    lbl_gem_sub = lv_label_create(card_gemini);
    lv_label_set_text(lbl_gem_sub, "00:00:00");
    lv_obj_set_style_text_color(lbl_gem_sub, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(lbl_gem_sub, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_gem_sub, LV_ALIGN_TOP_MID, 0, 114);

    lv_obj_t *lbl_gem_w = lv_label_create(card_gemini);
    lv_label_set_text(lbl_gem_w, "Wk");
    lv_obj_set_style_text_color(lbl_gem_w, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(lbl_gem_w, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_gem_w, LV_ALIGN_BOTTOM_LEFT, 10, -8);

    bar_gemini_w = lv_bar_create(card_gemini);
    lv_obj_set_size(bar_gemini_w, 94, 8);
    lv_bar_set_range(bar_gemini_w, 0, 100);
    lv_bar_set_value(bar_gemini_w, 71, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_gemini_w, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_gemini_w, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_gemini_w, 3, 0);
    lv_obj_align(bar_gemini_w, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    // Card Claude (Right)
    lv_obj_t *card_claude = lv_obj_create(scr_console);
    lv_obj_set_pos(card_claude, 164, 36);
    lv_obj_set_size(card_claude, 148, 156);
    lv_obj_set_style_bg_color(card_claude, lv_color_hex(0x162032), 0);
    lv_obj_set_style_bg_opa(card_claude, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card_claude, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(card_claude, 1, 0);
    lv_obj_set_style_radius(card_claude, 10, 0);
    lv_obj_set_style_pad_all(card_claude, 0, 0);
    lv_obj_clear_flag(card_claude, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_cld_title = lv_label_create(card_claude);
    lv_label_set_text(lbl_cld_title, "CLAUDE");
    lv_obj_set_style_text_color(lbl_cld_title, lv_color_hex(0xF97316), 0);
    lv_obj_set_style_text_font(lbl_cld_title, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_cld_title, LV_ALIGN_TOP_MID, 0, 6);

    arc_claude = lv_arc_create(card_claude);
    lv_obj_set_size(arc_claude, 88, 88);
    lv_arc_set_rotation(arc_claude, 135);
    lv_arc_set_bg_angles(arc_claude, 0, 270);
    lv_arc_set_value(arc_claude, 100);
    lv_obj_set_style_arc_color(arc_claude, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_claude, 7, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_claude, lv_color_hex(0xF97316), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_claude, 7, LV_PART_INDICATOR);
    lv_obj_remove_style(arc_claude, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_claude, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(arc_claude, LV_ALIGN_TOP_MID, 0, 26);

    lbl_claude_val = lv_label_create(arc_claude);
    lv_label_set_text(lbl_claude_val, "100%");
    lv_obj_set_style_text_color(lbl_claude_val, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_claude_val, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_claude_val);

    // Thời gian đếm ngược reset quota 5h Claude (thay thế chữ 5h Limit)
    lbl_cld_sub = lv_label_create(card_claude);
    lv_label_set_text(lbl_cld_sub, "00:00:00");
    lv_obj_set_style_text_color(lbl_cld_sub, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(lbl_cld_sub, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_cld_sub, LV_ALIGN_TOP_MID, 0, 114);

    lv_obj_t *lbl_cld_w = lv_label_create(card_claude);
    lv_label_set_text(lbl_cld_w, "Wk");
    lv_obj_set_style_text_color(lbl_cld_w, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(lbl_cld_w, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_cld_w, LV_ALIGN_BOTTOM_LEFT, 10, -8);

    bar_claude_w = lv_bar_create(card_claude);
    lv_obj_set_size(bar_claude_w, 94, 8);
    lv_bar_set_range(bar_claude_w, 0, 100);
    lv_bar_set_value(bar_claude_w, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_claude_w, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_claude_w, lv_color_hex(0xF97316), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_claude_w, 3, 0);
    lv_obj_align(bar_claude_w, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    // 3. BOTTOM BAR (Y = 196, Height = 44)
    lv_obj_t *bottom_bar = lv_obj_create(scr_console);
    lv_obj_set_pos(bottom_bar, 0, 196);
    lv_obj_set_size(bottom_bar, 320, 44);
    lv_obj_set_style_bg_color(bottom_bar, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bottom_bar, 0, 0);
    lv_obj_set_style_border_width(bottom_bar, 0, 0);
    lv_obj_set_style_pad_all(bottom_bar, 0, 0);
    lv_obj_clear_flag(bottom_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Nút HOME nhỏ
    lv_obj_t *btn_home = lv_btn_create(bottom_bar);
    lv_obj_set_size(btn_home, 48, 28);
    lv_obj_align(btn_home, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_bg_color(btn_home, lv_color_hex(0x334155), 0);
    lv_obj_set_style_radius(btn_home, 6, 0);
    lv_obj_set_style_pad_all(btn_home, 0, 0);
    lv_obj_add_event_cb(btn_home, nav_to_home_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_home_icon = lv_label_create(btn_home);
    lv_label_set_text(lbl_home_icon, LV_SYMBOL_HOME);
    lv_obj_set_style_text_color(lbl_home_icon, lv_color_hex(0x38BDF8), 0);
    lv_obj_center(lbl_home_icon);

    // Host PC CPU | RAM Telemetry
    lbl_system = lv_label_create(bottom_bar);
    lv_label_set_text(lbl_system, "CPU: --% | RAM: --%");
    lv_obj_set_style_text_color(lbl_system, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_text_font(lbl_system, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_system, LV_ALIGN_RIGHT_MID, -12, 0);

    // Chú cún Shiba Pixel Pet (48x47 px) to rõ, sinh động, nằm trên scr_console
    img_shiba = lv_img_create(scr_console);
    lv_img_set_src(img_shiba, shiba_anim_right[0]);
    lv_obj_set_pos(img_shiba, s_shiba_x, 191);
    lv_obj_add_flag(img_shiba, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(img_shiba, 6);
    lv_obj_add_event_cb(img_shiba, shiba_click_cb, LV_EVENT_CLICKED, NULL);
    if (!s_pet_enabled) {
        lv_obj_add_flag(img_shiba, LV_OBJ_FLAG_HIDDEN);
    }
}

// =========================================================================
// SCREEN 2: Home Screen (Giant Clock + Calendar + Mini Quota + Mode Buttons)
// =========================================================================
static void build_home_screen(void) {
    scr_home = lv_obj_create(NULL);
    lv_obj_clear_flag(scr_home, LV_OBJ_FLAG_SCROLLABLE);

    // 1. BACKGROUND: Hình nền Trái Đất chụp từ không gian (320x240)
    lv_obj_t *bg_img = lv_img_create(scr_home);
    lv_img_set_src(bg_img, &img_earth_wallpaper);
    lv_obj_set_pos(bg_img, 0, 0);
    lv_obj_set_size(bg_img, 320, 240);
    lv_obj_clear_flag(bg_img, LV_OBJ_FLAG_SCROLLABLE);

    // 2. FLOATING TOP CONTROLS
    // Nút chuyển lại Dashboard Console (Bỏ khung viền hoàn toàn theo yêu cầu)
    lv_obj_t *btn_back = lv_btn_create(scr_home);
    lv_obj_set_size(btn_back, 84, 26);
    lv_obj_set_pos(btn_back, 4, 4);
    lv_obj_set_style_bg_opa(btn_back, LV_OPA_TRANSP, 0);       // Bỏ màu nền
    lv_obj_set_style_border_width(btn_back, 0, 0);             // Bỏ khung viền
    lv_obj_set_style_shadow_width(btn_back, 0, 0);             // Bỏ bóng
    lv_obj_set_style_pad_all(btn_back, 0, 0);
    lv_obj_add_event_cb(btn_back, nav_to_console_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " QUOTA");
    lv_obj_set_style_text_color(lbl_back, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_back, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_back, LV_ALIGN_LEFT_MID, 6, 0);

    // Cột sóng BLE góc phải (Trong suốt, không viền)
    lv_obj_t *sig_pill = lv_obj_create(scr_home);
    lv_obj_set_size(sig_pill, 40, 24);
    lv_obj_set_pos(sig_pill, 272, 4);
    lv_obj_set_style_bg_opa(sig_pill, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sig_pill, 0, 0);
    lv_obj_set_style_pad_all(sig_pill, 0, 0);
    lv_obj_clear_flag(sig_pill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sig_cont = create_signal_widget(sig_pill, &h_bar_sig_1, &h_bar_sig_2, &h_bar_sig_3, &h_bar_sig_4, &h_lbl_sig_x);
    lv_obj_center(sig_cont);

    // 3. MODE SELECT BUTTONS (Bên trái màn hình Home: Gamepad & Keyboard+Mouse)
    btn_home_gp = lv_btn_create(scr_home);
    lv_obj_set_size(btn_home_gp, 44, 44);
    lv_obj_set_pos(btn_home_gp, 12, 48);
    lv_obj_set_style_radius(btn_home_gp, 10, 0);
    lv_obj_set_style_pad_all(btn_home_gp, 0, 0);
    lv_obj_clear_flag(btn_home_gp, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn_home_gp, nav_to_gamepad_cb, LV_EVENT_CLICKED, NULL);

    img_home_gp = lv_img_create(btn_home_gp);
    lv_img_set_src(img_home_gp, &img_icon_gamepad);
    lv_obj_center(img_home_gp);
    lv_obj_clear_flag(img_home_gp, LV_OBJ_FLAG_CLICKABLE);

    btn_home_km = lv_btn_create(scr_home);
    lv_obj_set_size(btn_home_km, 44, 44);
    lv_obj_set_pos(btn_home_km, 12, 102);
    lv_obj_set_style_radius(btn_home_km, 10, 0);
    lv_obj_set_style_pad_all(btn_home_km, 0, 0);
    lv_obj_clear_flag(btn_home_km, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn_home_km, nav_to_kb_mouse_cb, LV_EVENT_CLICKED, NULL);

    img_home_km = lv_img_create(btn_home_km);
    lv_img_set_src(img_home_km, &img_icon_kb_mouse);
    lv_obj_center(img_home_km);
    lv_obj_clear_flag(img_home_km, LV_OBJ_FLAG_CLICKABLE);

    // Cập nhật trạng thái viền/nền theo Mode đang kích hoạt
    display_ui_set_active_mode(s_active_mode);

    // 4. GIANT iPHONE CLOCK & DATE (Bố trí góc trên bên phải tương tự ảnh concept)
    lbl_home_big_clock = lv_label_create(scr_home);
    lv_label_set_text(lbl_home_big_clock, "21:49");
    lv_obj_set_style_text_color(lbl_home_big_clock, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_home_big_clock, &font_sf_pro_rounded_60, 0);
    lv_obj_align(lbl_home_big_clock, LV_ALIGN_TOP_RIGHT, -16, 62);

    // 5. DATE SUBTITLE (Nằm ngay trên đồng hồ, căn phải)
    lbl_home_calendar = lv_label_create(scr_home);
    lv_label_set_text(lbl_home_calendar, "CN,  6 thg 9");
    lv_obj_set_style_text_color(lbl_home_calendar, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_text_font(lbl_home_calendar, &font_sf_pro_rounded_16, 0);
    lv_obj_align_to(lbl_home_calendar, lbl_home_big_clock, LV_ALIGN_OUT_TOP_RIGHT, 0, -4);

    // 6. MINI QUOTA WIDGET (Thu gọn độ cao xuống 36px để tạo khoảng trống thoáng cho cún)
    lv_obj_t *panel_mini = lv_obj_create(scr_home);
    lv_obj_set_size(panel_mini, 304, 36);
    lv_obj_set_style_bg_color(panel_mini, lv_color_hex(0x0B1329), 0);
    lv_obj_set_style_bg_opa(panel_mini, LV_OPA_80, 0);
    lv_obj_set_style_border_color(panel_mini, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(panel_mini, 1, 0);
    lv_obj_set_style_radius(panel_mini, 10, 0);
    lv_obj_set_style_pad_all(panel_mini, 0, 0);
    lv_obj_align(panel_mini, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_clear_flag(panel_mini, LV_OBJ_FLAG_SCROLLABLE);

    // Gemini Mini (Căn thẳng hàng trên 1 dòng)
    lv_obj_t *lbl_mg = lv_label_create(panel_mini);
    lv_label_set_text(lbl_mg, "GEMINI");
    lv_obj_set_style_text_color(lbl_mg, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_mg, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_mg, 10, 10);

    bar_home_gemini = lv_bar_create(panel_mini);
    lv_obj_set_size(bar_home_gemini, 52, 6);
    lv_bar_set_range(bar_home_gemini, 0, 100);
    lv_bar_set_value(bar_home_gemini, 90, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_home_gemini, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_home_gemini, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_home_gemini, 3, 0);
    lv_obj_set_pos(bar_home_gemini, 66, 15);

    lbl_home_gem_val = lv_label_create(panel_mini);
    lv_label_set_text(lbl_home_gem_val, "90%");
    lv_obj_set_style_text_color(lbl_home_gem_val, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_home_gem_val, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_home_gem_val, 122, 10);

    // Vách ngăn giữa
    lv_obj_t *sep = lv_obj_create(panel_mini);
    lv_obj_set_size(sep, 1, 22);
    lv_obj_set_pos(sep, 152, 7);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    // Claude Mini (Căn thẳng hàng trên 1 dòng)
    lv_obj_t *lbl_mc = lv_label_create(panel_mini);
    lv_label_set_text(lbl_mc, "CLAUDE");
    lv_obj_set_style_text_color(lbl_mc, lv_color_hex(0xF97316), 0);
    lv_obj_set_style_text_font(lbl_mc, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_mc, 158, 10);

    bar_home_claude = lv_bar_create(panel_mini);
    lv_obj_set_size(bar_home_claude, 46, 6);
    lv_bar_set_range(bar_home_claude, 0, 100);
    lv_bar_set_value(bar_home_claude, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_home_claude, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_home_claude, lv_color_hex(0xF97316), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_home_claude, 3, 0);
    lv_obj_set_pos(bar_home_claude, 218, 15);

    lbl_home_cld_val = lv_label_create(panel_mini);
    lv_label_set_text(lbl_home_cld_val, "100%");
    lv_obj_set_style_text_color(lbl_home_cld_val, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_home_cld_val, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_home_cld_val, 268, 10);

    // 7. BONGO SHIBA (Chú cún gõ phím trên mép thanh Quota: 104x78 px, 3s gõ + 2s nghỉ)
    img_bongo_shiba = lv_img_create(scr_home);
    lv_img_set_src(img_bongo_shiba, &img_bongo_tap_l);
    lv_obj_set_pos(img_bongo_shiba, 52, 118);
    lv_obj_clear_flag(img_bongo_shiba, LV_OBJ_FLAG_CLICKABLE);
}

// =========================================================================
// SCREEN 3A: Dedicated Touchscreen Gamepad Window
// =========================================================================
static void build_gamepad_screen(void) {
    scr_gamepad = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_gamepad, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(scr_gamepad, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr_gamepad, LV_OBJ_FLAG_SCROLLABLE);

    // 1. TOP BAR
    lv_obj_t *top_bar = lv_obj_create(scr_gamepad);
    lv_obj_set_pos(top_bar, 0, 0);
    lv_obj_set_size(top_bar, 320, 30);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_back = lv_btn_create(top_bar);
    lv_obj_set_size(btn_back, 74, 24);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_bg_opa(btn_back, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_back, 0, 0);
    lv_obj_set_style_pad_all(btn_back, 0, 0);
    lv_obj_add_event_cb(btn_back, nav_back_to_home_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " HOME");
    lv_obj_set_style_text_color(lbl_back, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_back, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_back, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t *lbl_title = lv_label_create(top_bar);
    lv_label_set_text(lbl_title, "GAMEPAD (XBOX)");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xF1F5F9), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *sig_cont = create_signal_widget(top_bar, &gp_bar_sig_1, &gp_bar_sig_2, &gp_bar_sig_3, &gp_bar_sig_4, &gp_lbl_sig_x);
    lv_obj_align(sig_cont, LV_ALIGN_RIGHT_MID, -8, 0);

    // 2. BUMPERS & TRIGGERS (Row Y=36)
    create_control_btn(scr_gamepad, 10, 36, 52, 28, "LB", lv_color_hex(0x334155), lv_color_hex(0xE2E8F0),
                       gp_btn_action_cb, (void*)(uintptr_t)XBOX_BUTTON_L1, LV_EVENT_ALL);
    create_control_btn(scr_gamepad, 68, 36, 52, 28, "LT", lv_color_hex(0x1E293B), lv_color_hex(0x38BDF8),
                       gp_trigger_action_cb, (void*)(uintptr_t)1, LV_EVENT_ALL);
    create_control_btn(scr_gamepad, 200, 36, 52, 28, "RT", lv_color_hex(0x1E293B), lv_color_hex(0x38BDF8),
                       gp_trigger_action_cb, (void*)(uintptr_t)2, LV_EVENT_ALL);
    create_control_btn(scr_gamepad, 258, 36, 52, 28, "RB", lv_color_hex(0x334155), lv_color_hex(0xE2E8F0),
                       gp_btn_action_cb, (void*)(uintptr_t)XBOX_BUTTON_R1, LV_EVENT_ALL);

    // 3. CENTER: SELECT & START
    create_control_btn(scr_gamepad, 114, 96, 42, 28, "SEL", lv_color_hex(0x1E293B), lv_color_hex(0x94A3B8),
                       gp_btn_action_cb, (void*)(uintptr_t)XBOX_BUTTON_SELECT, LV_EVENT_ALL);
    create_control_btn(scr_gamepad, 164, 96, 42, 28, "STA", lv_color_hex(0x1E293B), lv_color_hex(0x94A3B8),
                       gp_btn_action_cb, (void*)(uintptr_t)XBOX_BUTTON_START, LV_EVENT_ALL);

    // 4. LEFT: D-PAD CROSS
    create_control_btn(scr_gamepad, 52, 78, 36, 36, LV_SYMBOL_UP, lv_color_hex(0x1E293B), lv_color_hex(0x38BDF8),
                       gp_dpad_action_cb, (void*)(uintptr_t)GAMEPAD_DPAD_UP, LV_EVENT_ALL);
    create_control_btn(scr_gamepad, 12, 118, 36, 36, LV_SYMBOL_LEFT, lv_color_hex(0x1E293B), lv_color_hex(0x38BDF8),
                       gp_dpad_action_cb, (void*)(uintptr_t)GAMEPAD_DPAD_LEFT, LV_EVENT_ALL);
    create_control_btn(scr_gamepad, 92, 118, 36, 36, LV_SYMBOL_RIGHT, lv_color_hex(0x1E293B), lv_color_hex(0x38BDF8),
                       gp_dpad_action_cb, (void*)(uintptr_t)GAMEPAD_DPAD_RIGHT, LV_EVENT_ALL);
    create_control_btn(scr_gamepad, 52, 158, 36, 36, LV_SYMBOL_DOWN, lv_color_hex(0x1E293B), lv_color_hex(0x38BDF8),
                       gp_dpad_action_cb, (void*)(uintptr_t)GAMEPAD_DPAD_DOWN, LV_EVENT_ALL);

    lv_obj_t *dpad_center = lv_obj_create(scr_gamepad);
    lv_obj_set_pos(dpad_center, 56, 122);
    lv_obj_set_size(dpad_center, 28, 28);
    lv_obj_set_style_bg_color(dpad_center, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_border_width(dpad_center, 0, 0);
    lv_obj_set_style_radius(dpad_center, 4, 0);
    lv_obj_clear_flag(dpad_center, LV_OBJ_FLAG_CLICKABLE);

    // 5. RIGHT: ACTION BUTTONS (Y, X, B, A in Xbox Colors)
    create_control_btn(scr_gamepad, 232, 78, 38, 38, "Y", lv_color_hex(0xEAB308), lv_color_black(),
                       gp_btn_action_cb, (void*)(uintptr_t)XBOX_BUTTON_Y, LV_EVENT_ALL);
    create_control_btn(scr_gamepad, 190, 118, 38, 38, "X", lv_color_hex(0x3B82F6), lv_color_white(),
                       gp_btn_action_cb, (void*)(uintptr_t)XBOX_BUTTON_X, LV_EVENT_ALL);
    create_control_btn(scr_gamepad, 274, 118, 38, 38, "B", lv_color_hex(0xEF4444), lv_color_white(),
                       gp_btn_action_cb, (void*)(uintptr_t)XBOX_BUTTON_B, LV_EVENT_ALL);
    create_control_btn(scr_gamepad, 232, 158, 38, 38, "A", lv_color_hex(0x22C55E), lv_color_black(),
                       gp_btn_action_cb, (void*)(uintptr_t)XBOX_BUTTON_A, LV_EVENT_ALL);

    // Bottom Hint Label
    lv_obj_t *lbl_hint = lv_label_create(scr_gamepad);
    lv_label_set_text(lbl_hint, "Touch screen to control Gamepad");
    lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_hint, LV_ALIGN_BOTTOM_MID, 0, -8);
}

// =========================================================================
// SCREEN 3B: Dedicated Touchscreen Keyboard & Mouse Window
// =========================================================================
static void build_kb_mouse_screen(void) {
    scr_kb_mouse = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_kb_mouse, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(scr_kb_mouse, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr_kb_mouse, LV_OBJ_FLAG_SCROLLABLE);

    // 1. TOP BAR
    lv_obj_t *top_bar = lv_obj_create(scr_kb_mouse);
    lv_obj_set_pos(top_bar, 0, 0);
    lv_obj_set_size(top_bar, 320, 30);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_back = lv_btn_create(top_bar);
    lv_obj_set_size(btn_back, 74, 24);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_bg_opa(btn_back, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_back, 0, 0);
    lv_obj_set_style_pad_all(btn_back, 0, 0);
    lv_obj_add_event_cb(btn_back, nav_back_to_home_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " HOME");
    lv_obj_set_style_text_color(lbl_back, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_back, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_back, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t *lbl_title = lv_label_create(top_bar);
    lv_label_set_text(lbl_title, "KEYBOARD & MOUSE");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x10B981), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *sig_cont = create_signal_widget(top_bar, &km_bar_sig_1, &km_bar_sig_2, &km_bar_sig_3, &km_bar_sig_4, &km_lbl_sig_x);
    lv_obj_align(sig_cont, LV_ALIGN_RIGHT_MID, -8, 0);

    // 2. LEFT: TRACKPAD & MOUSE (X=8, Width 140)
    lv_obj_t *trackpad = lv_obj_create(scr_kb_mouse);
    lv_obj_set_pos(trackpad, 8, 34);
    lv_obj_set_size(trackpad, 140, 122);
    lv_obj_set_style_bg_color(trackpad, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(trackpad, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(trackpad, lv_color_hex(0x475569), 0);
    lv_obj_set_style_border_width(trackpad, 1, 0);
    lv_obj_set_style_radius(trackpad, 8, 0);
    lv_obj_set_style_pad_all(trackpad, 0, 0);
    lv_obj_clear_flag(trackpad, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(trackpad, trackpad_touch_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *lbl_tp = lv_label_create(trackpad);
    lv_label_set_text(lbl_tp, "TRACKPAD\nDrag to Move");
    lv_obj_set_style_text_color(lbl_tp, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(lbl_tp, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(lbl_tp, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lbl_tp);

    // Left Click & Right Click
    create_control_btn(scr_kb_mouse, 8, 160, 68, 32, "L-CLICK", lv_color_hex(0x0284C7), lv_color_white(),
                       mouse_btn_action_cb, (void*)(uintptr_t)MOUSE_BUTTON_LEFT, LV_EVENT_ALL);
    create_control_btn(scr_kb_mouse, 80, 160, 68, 32, "R-CLICK", lv_color_hex(0xD97706), lv_color_white(),
                       mouse_btn_action_cb, (void*)(uintptr_t)MOUSE_BUTTON_RIGHT, LV_EVENT_ALL);

    // Scroll Up & Scroll Down
    create_control_btn(scr_kb_mouse, 8, 196, 68, 32, LV_SYMBOL_UP " SCRL", lv_color_hex(0x1E293B), lv_color_hex(0x38BDF8),
                       mouse_scroll_action_cb, (void*)(intptr_t)1, LV_EVENT_CLICKED);
    create_control_btn(scr_kb_mouse, 80, 196, 68, 32, LV_SYMBOL_DOWN " SCRL", lv_color_hex(0x1E293B), lv_color_hex(0x38BDF8),
                       mouse_scroll_action_cb, (void*)(intptr_t)-1, LV_EVENT_CLICKED);

    // 3. RIGHT: KEYBOARD & SHORTCUTS (X=156)
    create_control_btn(scr_kb_mouse, 156, 34, 36, 28, "ESC", lv_color_hex(0xDC2626), lv_color_white(),
                       kb_key_action_cb, (void*)(uintptr_t)HID_KEY_ESCAPE, LV_EVENT_ALL);
    create_control_btn(scr_kb_mouse, 195, 34, 36, 28, "TAB", lv_color_hex(0x334155), lv_color_white(),
                       kb_key_action_cb, (void*)(uintptr_t)HID_KEY_TAB, LV_EVENT_ALL);
    create_control_btn(scr_kb_mouse, 234, 34, 38, 28, "ENT", lv_color_hex(0x15803D), lv_color_white(),
                       kb_key_action_cb, (void*)(uintptr_t)HID_KEY_ENTER, LV_EVENT_ALL);
    create_control_btn(scr_kb_mouse, 275, 34, 37, 28, "DEL", lv_color_hex(0x334155), lv_color_white(),
                       kb_key_action_cb, (void*)(uintptr_t)0x4C, LV_EVENT_ALL);

    create_control_btn(scr_kb_mouse, 156, 66, 75, 28, "SPACE", lv_color_hex(0x334155), lv_color_white(),
                       kb_key_action_cb, (void*)(uintptr_t)HID_KEY_SPACE, LV_EVENT_ALL);
    create_control_btn(scr_kb_mouse, 234, 66, 38, 28, "WIN", lv_color_hex(0x2563EB), lv_color_white(),
                       kb_key_action_cb, (void*)(uintptr_t)((KEY_MOD_LMETA << 8) | 0), LV_EVENT_ALL);
    create_control_btn(scr_kb_mouse, 275, 66, 37, 28, "BS", lv_color_hex(0x334155), lv_color_white(),
                       kb_key_action_cb, (void*)(uintptr_t)HID_KEY_BACKSPACE, LV_EVENT_ALL);

    create_control_btn(scr_kb_mouse, 156, 98, 48, 28, "COPY", lv_color_hex(0x4338CA), lv_color_white(),
                       kb_key_action_cb, (void*)(uintptr_t)((KEY_MOD_LCTRL << 8) | HID_KEY_C), LV_EVENT_ALL);
    create_control_btn(scr_kb_mouse, 208, 98, 48, 28, "PASTE", lv_color_hex(0x6D28D9), lv_color_white(),
                       kb_key_action_cb, (void*)(uintptr_t)((KEY_MOD_LCTRL << 8) | 0x19), LV_EVENT_ALL);
    create_control_btn(scr_kb_mouse, 260, 98, 52, 28, LV_SYMBOL_KEYBOARD " KB", lv_color_hex(0x0284C7), lv_color_white(),
                       nav_to_full_kb_cb, NULL, LV_EVENT_CLICKED);

    lv_obj_t *h_sep = lv_obj_create(scr_kb_mouse);
    lv_obj_set_pos(h_sep, 156, 130);
    lv_obj_set_size(h_sep, 156, 1);
    lv_obj_set_style_bg_color(h_sep, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(h_sep, 0, 0);

    lv_obj_t *lbl_med = lv_label_create(scr_kb_mouse);
    lv_label_set_text(lbl_med, "MEDIA CONTROLS");
    lv_obj_set_style_text_color(lbl_med, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(lbl_med, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_med, 156, 138);

    // Loa bật/tắt (Toggle speaker icon thay vì chữ MUTE)
    lv_obj_t *btn_mute = lv_btn_create(scr_kb_mouse);
    lv_obj_set_pos(btn_mute, 156, 158);
    lv_obj_set_size(btn_mute, 36, 32);
    lv_obj_set_style_bg_color(btn_mute, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_radius(btn_mute, 6, 0);
    lv_obj_set_style_pad_all(btn_mute, 0, 0);
    lv_obj_set_style_border_color(btn_mute, lv_color_hex(0x475569), 0);
    lv_obj_set_style_border_width(btn_mute, 1, 0);
    lv_obj_clear_flag(btn_mute, LV_OBJ_FLAG_SCROLLABLE);

    lbl_mute_icon = lv_label_create(btn_mute);
    lv_label_set_text(lbl_mute_icon, s_is_muted ? LV_SYMBOL_MUTE : LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_text_color(lbl_mute_icon, s_is_muted ? lv_color_hex(0xEF4444) : lv_color_hex(0x10B981), 0);
    lv_obj_set_style_text_font(lbl_mute_icon, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_mute_icon);

    lv_obj_add_event_cb(btn_mute, mute_btn_action_cb, LV_EVENT_ALL, NULL);

    create_control_btn(scr_kb_mouse, 195, 158, 36, 32, "V -", lv_color_hex(0x1E293B), lv_color_hex(0x38BDF8),
                       media_key_action_cb, (void*)(uintptr_t)CONSUMER_VOLUME_DOWN, LV_EVENT_ALL);
    create_control_btn(scr_kb_mouse, 234, 158, 38, 32, "V +", lv_color_hex(0x1E293B), lv_color_hex(0x38BDF8),
                       media_key_action_cb, (void*)(uintptr_t)CONSUMER_VOLUME_UP, LV_EVENT_ALL);
    create_control_btn(scr_kb_mouse, 275, 158, 37, 32, LV_SYMBOL_PLAY, lv_color_hex(0x1E293B), lv_color_hex(0x10B981),
                       media_key_action_cb, (void*)(uintptr_t)CONSUMER_PLAY_PAUSE, LV_EVENT_ALL);

    create_control_btn(scr_kb_mouse, 156, 196, 75, 32, LV_SYMBOL_PREV " PREV", lv_color_hex(0x1E293B), lv_color_hex(0xCBD5E1),
                       media_key_action_cb, (void*)(uintptr_t)CONSUMER_SCAN_PREV, LV_EVENT_ALL);
    create_control_btn(scr_kb_mouse, 234, 196, 78, 32, "NEXT " LV_SYMBOL_NEXT, lv_color_hex(0x1E293B), lv_color_hex(0xCBD5E1),
                       media_key_action_cb, (void*)(uintptr_t)CONSUMER_SCAN_NEXT, LV_EVENT_ALL);
}

// =========================================================================
// SCREEN 3C: Dedicated Full Virtual Keyboard Window
// =========================================================================
static void build_full_kb_screen(void) {
    scr_full_kb = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_full_kb, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(scr_full_kb, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr_full_kb, LV_OBJ_FLAG_SCROLLABLE);

    // Textarea ẩn (off-screen) — LVGL keyboard cần có textarea để chuyển mode ABC/1# hoạt động đúng
    ta_full_kb = lv_textarea_create(scr_full_kb);
    lv_obj_set_pos(ta_full_kb, -200, -200);
    lv_obj_set_size(ta_full_kb, 1, 1);
    lv_textarea_set_max_length(ta_full_kb, 32);
    lv_obj_add_flag(ta_full_kb, LV_OBJ_FLAG_HIDDEN);

    // TOP BAR (12px) — nút BACK + sóng BLE thu nhỏ gọn
    lv_obj_t *top_bar = lv_obj_create(scr_full_kb);
    lv_obj_set_pos(top_bar, 0, 0);
    lv_obj_set_size(top_bar, 320, 12);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_back = lv_btn_create(top_bar);
    lv_obj_set_size(btn_back, 50, 12);
    lv_obj_set_pos(btn_back, 2, 0);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x334155), 0);
    lv_obj_set_style_radius(btn_back, 3, 0);
    lv_obj_set_style_pad_all(btn_back, 0, 0);
    lv_obj_add_event_cb(btn_back, nav_back_to_kb_mouse_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " BACK");
    lv_obj_set_style_text_color(lbl_back, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_back, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_back);

    // Mini signal widget trong thanh 12px
    lv_obj_t *sig_cont = lv_obj_create(top_bar);
    lv_obj_set_size(sig_cont, 26, 11);
    lv_obj_set_pos(sig_cont, 288, 0);
    lv_obj_set_style_bg_opa(sig_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sig_cont, 0, 0);
    lv_obj_set_style_pad_all(sig_cont, 0, 0);
    lv_obj_clear_flag(sig_cont, LV_OBJ_FLAG_SCROLLABLE);

    int sig_h[] = {2, 4, 6, 8};
    int sig_x[] = {1, 6, 11, 16};
    lv_obj_t **fkb_bars[] = {&fkb_bar_sig_1, &fkb_bar_sig_2, &fkb_bar_sig_3, &fkb_bar_sig_4};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *bar = lv_obj_create(sig_cont);
        lv_obj_set_pos(bar, sig_x[i], 10 - sig_h[i]);
        lv_obj_set_size(bar, 3, sig_h[i]);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x475569), 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bar, 1, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
        *fkb_bars[i] = bar;
    }

    fkb_lbl_sig_x = lv_label_create(sig_cont);
    lv_label_set_text(fkb_lbl_sig_x, "x");
    lv_obj_set_style_text_color(fkb_lbl_sig_x, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_text_font(fkb_lbl_sig_x, &lv_font_montserrat_14, 0);
    lv_obj_align(fkb_lbl_sig_x, LV_ALIGN_CENTER, 0, 0);

    // BÀN PHÍM QWERTY — dịch lên 10px (bắt đầu từ Y=12 thay vì Y=22)
    lv_obj_t *kb = lv_keyboard_create(scr_full_kb);
    lv_obj_set_size(kb, 320, 218);
    lv_obj_set_pos(kb, 0, 12);
    lv_keyboard_set_textarea(kb, ta_full_kb);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(kb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(kb, 0, 0);
    lv_obj_set_style_pad_row(kb, 3, 0);
    lv_obj_set_style_pad_column(kb, 3, 0);
    lv_obj_set_style_pad_left(kb, 4, 0);
    lv_obj_set_style_pad_right(kb, 4, 0);
    lv_obj_set_style_pad_top(kb, 3, 0);
    lv_obj_set_style_pad_bottom(kb, 3, 0);

    // Style phím
    lv_obj_set_style_bg_color(kb, lv_color_hex(0x1E293B), LV_PART_ITEMS);
    lv_obj_set_style_text_color(kb, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_font(kb, &lv_font_montserrat_16, LV_PART_ITEMS);
    lv_obj_set_style_border_color(kb, lv_color_hex(0x334155), LV_PART_ITEMS);
    lv_obj_set_style_border_width(kb, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(kb, 5, LV_PART_ITEMS);

    lv_obj_add_event_cb(kb, full_kb_action_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

// =========================================================================
// Real-Time Quota & Telemetry Data Update Function
// =========================================================================
esp_err_t display_ui_update_quota(const pc_quota_packet_t *quota) {
    if (!quota) return ESP_ERR_INVALID_ARG;

    s_last_packet_time_ms = (uint32_t)(esp_timer_get_time() / 1000);
    s_ble_connected = true;

    // 1. Đồng bộ thời gian nội tại của ESP32 (Sử dụng timestamp epoch hoặc parse HH:MM:SS)
    if (quota->epoch_sec > 1000000000) {
        struct timeval tv = { .tv_sec = quota->epoch_sec, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        s_clock_synced = true;
    } else if (quota->time_vn[0] != '\0') {
        int h = 0, m = 0, s = 0;
        if (sscanf(quota->time_vn, "%d:%d:%d", &h, &m, &s) >= 2) {
            time_t now = time(NULL);
            struct tm ti;
            localtime_r(&now, &ti);
            if (ti.tm_year < 120) {
                ti.tm_year = 126; // 2026
                ti.tm_mon = 8;    // Tháng 9 (0-indexed: 8)
                ti.tm_mday = 6;
            }
            ti.tm_hour = h;
            ti.tm_min = m;
            ti.tm_sec = s;
            time_t new_now = mktime(&ti);
            struct timeval tv = { .tv_sec = new_now, .tv_usec = 0 };
            settimeofday(&tv, NULL);
            s_clock_synced = true;
        }
    }

    s_gemini_quota_pct = quota->antigrav_quota_percent;
    s_claude_quota_pct = quota->claude_5h_percent;

    // Parse thời gian đếm lùi reset Quota 5h (hỗ trợ cả HH:MM:SS và HH:MM)
    if (quota->gemini_reset[0] != '\0') {
        int rh = 0, rm = 0, rs = 0;
        int parsed = sscanf(quota->gemini_reset, "%d:%d:%d", &rh, &rm, &rs);
        if (parsed == 3) {
            int32_t new_sec = rh * 3600 + rm * 60 + rs;
            if (s_gemini_reset_sec < 0 || abs((int)(s_gemini_reset_sec - new_sec)) > 3) {
                s_gemini_reset_sec = new_sec;
            }
        } else if (parsed == 2) {
            int32_t new_sec = rh * 3600 + rm * 60;
            if (s_gemini_reset_sec < 0 || abs((int)(s_gemini_reset_sec - new_sec)) > 3) {
                s_gemini_reset_sec = new_sec;
            }
        }
    }
    if (quota->claude_reset[0] != '\0') {
        int rh = 0, rm = 0, rs = 0;
        int parsed = sscanf(quota->claude_reset, "%d:%d:%d", &rh, &rm, &rs);
        if (parsed == 3) {
            int32_t new_sec = rh * 3600 + rm * 60 + rs;
            if (s_claude_reset_sec < 0 || abs((int)(s_claude_reset_sec - new_sec)) > 3) {
                s_claude_reset_sec = new_sec;
            }
        } else if (parsed == 2) {
            int32_t new_sec = rh * 3600 + rm * 60;
            if (s_claude_reset_sec < 0 || abs((int)(s_claude_reset_sec - new_sec)) > 3) {
                s_claude_reset_sec = new_sec;
            }
        }
    }

    if (lvgl_lock(100)) {
        char buf[64];

        // 1. Đồng hồ & Ngày tháng (Cập nhật cho cả 2 màn hình)
        if (quota->time_vn[0] != '\0') {
            if (lbl_clock) lv_label_set_text(lbl_clock, quota->time_vn);
            if (lbl_home_big_clock) {
                // Trích xuất định dạng HH:MM cho đồng hồ iPhone
                char time_hm[8];
                if (strlen(quota->time_vn) >= 5) {
                    strncpy(time_hm, quota->time_vn, 5);
                    time_hm[5] = '\0';
                } else {
                    strncpy(time_hm, quota->time_vn, sizeof(time_hm) - 1);
                    time_hm[sizeof(time_hm) - 1] = '\0';
                }
                strncpy(s_last_rendered_hm, time_hm, sizeof(s_last_rendered_hm) - 1);
                lv_label_set_text(lbl_home_big_clock, time_hm);
            }
        }
        if (quota->date_vn[0] != '\0') {
            if (lbl_home_calendar) lv_label_set_text(lbl_home_calendar, quota->date_vn);
        }

        // 2. Gemini Quota (Console Card + Home Mini)
        if (arc_gemini && lbl_gemini_val) {
            lv_arc_set_value(arc_gemini, quota->antigrav_quota_percent);
            snprintf(buf, sizeof(buf), "%d%%", quota->antigrav_quota_percent);
            lv_label_set_text(lbl_gemini_val, buf);

            // Đổi màu cảnh báo nếu quota thấp
            if (quota->antigrav_quota_percent < 20) {
                lv_obj_set_style_arc_color(arc_gemini, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
            } else if (quota->antigrav_quota_percent < 50) {
                lv_obj_set_style_arc_color(arc_gemini, lv_palette_main(LV_PALETTE_AMBER), LV_PART_INDICATOR);
            } else {
                lv_obj_set_style_arc_color(arc_gemini, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
            }
        }
        if (bar_gemini_w) {
            lv_bar_set_value(bar_gemini_w, quota->gemini_weekly_percent, LV_ANIM_OFF);
        }
        if (lbl_gem_sub) {
            if (s_gemini_reset_sec >= 0) {
                snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                         (int)(s_gemini_reset_sec / 3600),
                         (int)((s_gemini_reset_sec % 3600) / 60),
                         (int)(s_gemini_reset_sec % 60));
                lv_label_set_text(lbl_gem_sub, buf);
            } else if (quota->gemini_reset[0] != '\0') {
                lv_label_set_text(lbl_gem_sub, quota->gemini_reset);
            }
        }
        if (bar_home_gemini && lbl_home_gem_val) {
            lv_bar_set_value(bar_home_gemini, quota->antigrav_quota_percent, LV_ANIM_OFF);
            snprintf(buf, sizeof(buf), "%d%%", quota->antigrav_quota_percent);
            lv_label_set_text(lbl_home_gem_val, buf);
        }

        // 3. Claude Quota (Console Card + Home Mini)
        if (arc_claude && lbl_claude_val) {
            lv_arc_set_value(arc_claude, quota->claude_5h_percent);
            snprintf(buf, sizeof(buf), "%d%%", quota->claude_5h_percent);
            lv_label_set_text(lbl_claude_val, buf);
        }
        if (bar_claude_w) {
            lv_bar_set_value(bar_claude_w, quota->claude_weekly_percent, LV_ANIM_OFF);
        }
        if (lbl_cld_sub) {
            if (s_claude_reset_sec >= 0) {
                snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                         (int)(s_claude_reset_sec / 3600),
                         (int)((s_claude_reset_sec % 3600) / 60),
                         (int)(s_claude_reset_sec % 60));
                lv_label_set_text(lbl_cld_sub, buf);
            } else if (quota->claude_reset[0] != '\0') {
                lv_label_set_text(lbl_cld_sub, quota->claude_reset);
            }
        }
        if (bar_home_claude && lbl_home_cld_val) {
            lv_bar_set_value(bar_home_claude, quota->claude_5h_percent, LV_ANIM_OFF);
            snprintf(buf, sizeof(buf), "%d%%", quota->claude_5h_percent);
            lv_label_set_text(lbl_home_cld_val, buf);
        }

        // 4. Host PC CPU / RAM Telemetry
        if (lbl_system) {
            snprintf(buf, sizeof(buf), "CPU: %d%% | RAM: %d%%", quota->cpu_percent, quota->ram_percent);
            lv_label_set_text(lbl_system, buf);
        }

        // 5. Cột sóng BLE: 4 vạch xanh (Đã kết nối và đồng bộ)
        set_signal_level(4);

        lvgl_unlock();
    }

    return ESP_OK;
}

// =========================================================================
// Initialization
// =========================================================================
esp_err_t display_ui_init(void) {
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "  DESK CONSOLE: MULTI-SCREEN (DASHBOARD + HOME)  ");
    ESP_LOGI(TAG, "=================================================");

    // 0. Cấu hình múi giờ Việt Nam (UTC+7) cho POSIX time
    setenv("TZ", "ICT-7", 1);
    tzset();

    // 1. Backlight ON
    gpio_config_t bk_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BL,
    };
    gpio_config(&bk_conf);
    gpio_set_level(PIN_NUM_BL, 1);

    // 2. DC and RST GPIO
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST),
    };
    gpio_config(&io_conf);

    // 3. Hardware Reset (50ms low, 150ms high)
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    // 4. SPI Bus init
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LVGL_BUFFER_LINES * sizeof(lv_color_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 5. SPI Device (20MHz Mode 0)
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 20 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &devcfg, &spi_dev));

    // 6. EXACT HARDWARE INIT (100% verified configuration)
    send_cmd(0x11); // SLPOUT
    vTaskDelay(pdMS_TO_TICKS(120));

    send_cmd(0x3A); // COLMOD 16-bit RGB565
    uint8_t colmod[] = { 0x55 };
    send_data(colmod, 1);

    send_cmd(0x36); // MADCTL = 0xA0 (Landscape 320x240, Header on LEFT, RGB order)
    uint8_t madctl[] = { 0xA0 };
    send_data(madctl, 1);

    send_cmd(0x29); // DISPON
    vTaskDelay(pdMS_TO_TICKS(50));

    // 7. LVGL Engine Init
    lv_init();

    lv_color_t *buf1 = heap_caps_malloc(LCD_H_RES * LVGL_BUFFER_LINES * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(LCD_H_RES * LVGL_BUFFER_LINES * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * LVGL_BUFFER_LINES);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_handle = lv_disp_drv_register(&disp_drv);
    assert(disp_handle);

    // 8. LVGL Tick Timer
    const esp_timer_create_args_t tick_args = {
        .callback = &lvgl_tick_cb,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    // 9. LVGL Mutex & FreeRTOS Task
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);
    xTaskCreate(lvgl_task, "LVGL", 4 * 1024, NULL, 2, NULL);

    // 10. Build Screens (Console + Home + Gamepad + Keyboard/Mouse + Full Keyboard)
    if (lvgl_lock(-1)) {
        load_pet_setting();
        build_console_screen();
        build_home_screen();
        build_gamepad_screen();
        build_kb_mouse_screen();
        build_full_kb_screen();
        lv_scr_load(scr_console); // Mặc định mở màn hình Dashboard Quota
        set_signal_level(0);       // Khởi động chưa có BLE: hiện dấu x đỏ

        // 11. Cảm ứng XPT2046
        touch_xpt2046_init();
        touch_xpt2046_register_lvgl();

        // 12. Tạo timer 1s để ESP32 tự đếm giây và duy trì đồng hồ khi ngắt kết nối
        lv_timer_create(clock_tick_timer_cb, 1000, NULL);
        // 13. Timer hoạt ảnh chú cún Shiba Pixel Pet (110ms)
        lv_timer_create(shiba_anim_timer_cb, 110, NULL);
        // 14. Timer hoạt ảnh chú cún Bongo Shiba gõ phím trên màn hình Home (120ms)
        lv_timer_create(bongo_shiba_anim_timer_cb, 120, NULL);

        lvgl_unlock();
    }

    ESP_LOGI(TAG, "Multi-Screen Dashboard UI initialized successfully!");
    return ESP_OK;
}
