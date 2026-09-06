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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"

#include "display_ui.h"
#include "touch_xpt2046.h"

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

// 2 Screen Handles
static lv_obj_t *scr_console = NULL;
static lv_obj_t *scr_home = NULL;

// Screen 1 (Console) Widget Pointers
static lv_obj_t *lbl_clock = NULL;
static lv_obj_t *arc_gemini = NULL;
static lv_obj_t *lbl_gemini_val = NULL;
static lv_obj_t *bar_gemini_w = NULL;

static lv_obj_t *arc_claude = NULL;
static lv_obj_t *lbl_claude_val = NULL;
static lv_obj_t *bar_claude_w = NULL;

static lv_obj_t *lbl_system = NULL;

// Signal Bars (Screen 1 & Screen 2)
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

// Screen 2 (Home) Widget Pointers
static lv_obj_t *lbl_home_big_clock = NULL;
static lv_obj_t *lbl_home_calendar = NULL;
static lv_obj_t *bar_home_gemini = NULL;
static lv_obj_t *lbl_home_gem_val = NULL;
static lv_obj_t *bar_home_claude = NULL;
static lv_obj_t *lbl_home_cld_val = NULL;

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
    lv_color_t c_on = lv_palette_main(LV_PALETTE_GREEN);
    lv_color_t c_off = lv_color_hex(0x475569);

    lv_obj_t *b1_arr[] = {bar_sig_1, h_bar_sig_1};
    lv_obj_t *b2_arr[] = {bar_sig_2, h_bar_sig_2};
    lv_obj_t *b3_arr[] = {bar_sig_3, h_bar_sig_3};
    lv_obj_t *b4_arr[] = {bar_sig_4, h_bar_sig_4};
    lv_obj_t *x_arr[]  = {lbl_sig_x, h_lbl_sig_x};

    for (int s = 0; s < 2; s++) {
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

esp_err_t display_ui_toggle_screen(void) {
    if (lvgl_lock(100)) {
        lv_obj_t *act = lv_scr_act();
        if (act == scr_console && scr_home) {
            lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, false);
        } else if (act == scr_home && scr_console) {
            lv_scr_load_anim(scr_console, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 250, 0, false);
        }
        lvgl_unlock();
    }
    return ESP_OK;
}

// =========================================================================
// SCREEN 1: Quota Dashboard Console
// =========================================================================
static void build_console_screen(void) {
    scr_console = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_console, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(scr_console, LV_OPA_COVER, 0);

    // 1. TOP BAR (Height: 30px)
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

    // BLE Signal Bars (Replaces text indicator)
    lv_obj_t *sig_cont = create_signal_widget(top_bar, &bar_sig_1, &bar_sig_2, &bar_sig_3, &bar_sig_4, &lbl_sig_x);
    lv_obj_align(sig_cont, LV_ALIGN_RIGHT_MID, -12, 0);

    // 2. MIDDLE AREA: 2 CARDS (Y = 36, Height = 168)
    // Card Gemini (Left)
    lv_obj_t *card_gemini = lv_obj_create(scr_console);
    lv_obj_set_pos(card_gemini, 8, 36);
    lv_obj_set_size(card_gemini, 148, 168);
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

    lv_obj_t *lbl_gem_sub = lv_label_create(card_gemini);
    lv_label_set_text(lbl_gem_sub, "5h Limit");
    lv_obj_set_style_text_color(lbl_gem_sub, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(lbl_gem_sub, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_gem_sub, LV_ALIGN_TOP_MID, 0, 118);

    lv_obj_t *lbl_gem_w = lv_label_create(card_gemini);
    lv_label_set_text(lbl_gem_w, "Wk");
    lv_obj_set_style_text_color(lbl_gem_w, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(lbl_gem_w, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_gem_w, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    bar_gemini_w = lv_bar_create(card_gemini);
    lv_obj_set_size(bar_gemini_w, 94, 8);
    lv_bar_set_range(bar_gemini_w, 0, 100);
    lv_bar_set_value(bar_gemini_w, 71, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_gemini_w, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_gemini_w, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_gemini_w, 3, 0);
    lv_obj_align(bar_gemini_w, LV_ALIGN_BOTTOM_RIGHT, -10, -12);

    // Card Claude (Right)
    lv_obj_t *card_claude = lv_obj_create(scr_console);
    lv_obj_set_pos(card_claude, 164, 36);
    lv_obj_set_size(card_claude, 148, 168);
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

    lv_obj_t *lbl_cld_sub = lv_label_create(card_claude);
    lv_label_set_text(lbl_cld_sub, "5h Limit");
    lv_obj_set_style_text_color(lbl_cld_sub, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(lbl_cld_sub, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_cld_sub, LV_ALIGN_TOP_MID, 0, 118);

    lv_obj_t *lbl_cld_w = lv_label_create(card_claude);
    lv_label_set_text(lbl_cld_w, "Wk");
    lv_obj_set_style_text_color(lbl_cld_w, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(lbl_cld_w, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_cld_w, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    bar_claude_w = lv_bar_create(card_claude);
    lv_obj_set_size(bar_claude_w, 94, 8);
    lv_bar_set_range(bar_claude_w, 0, 100);
    lv_bar_set_value(bar_claude_w, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_claude_w, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_claude_w, lv_color_hex(0xF97316), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_claude_w, 3, 0);
    lv_obj_align(bar_claude_w, LV_ALIGN_BOTTOM_RIGHT, -10, -12);

    // 3. BOTTOM BAR (Y = 208, Height = 32)
    lv_obj_t *bottom_bar = lv_obj_create(scr_console);
    lv_obj_set_pos(bottom_bar, 0, 208);
    lv_obj_set_size(bottom_bar, 320, 32);
    lv_obj_set_style_bg_color(bottom_bar, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bottom_bar, 0, 0);
    lv_obj_set_style_border_width(bottom_bar, 0, 0);
    lv_obj_set_style_pad_all(bottom_bar, 0, 0);
    lv_obj_clear_flag(bottom_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Nút HOME nhỏ (Thay thế thông số Prompts)
    lv_obj_t *btn_home = lv_btn_create(bottom_bar);
    lv_obj_set_size(btn_home, 48, 24);
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
}

// =========================================================================
// SCREEN 2: Home Screen (Giant Clock + Calendar + Mini Quota)
// =========================================================================
static void build_home_screen(void) {
    scr_home = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_home, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(scr_home, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr_home, LV_OBJ_FLAG_SCROLLABLE);

    // Cho phép chạm vào bất kỳ đâu trên màn hình Home để chuyển về Console
    lv_obj_add_event_cb(scr_home, nav_to_console_cb, LV_EVENT_CLICKED, NULL);

    // 1. TOP BAR (Height: 28px)
    lv_obj_t *top_bar = lv_obj_create(scr_home);
    lv_obj_set_pos(top_bar, 0, 0);
    lv_obj_set_size(top_bar, 320, 28);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Nút chuyển lại Dashboard Console
    lv_obj_t *btn_back = lv_btn_create(top_bar);
    lv_obj_set_size(btn_back, 74, 22);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x334155), 0);
    lv_obj_set_style_radius(btn_back, 4, 0);
    lv_obj_set_style_pad_all(btn_back, 0, 0);
    lv_obj_add_event_cb(btn_back, nav_to_console_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " QUOTA");
    lv_obj_set_style_text_color(lbl_back, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_back, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_back);

    // Cột sóng BLE góc phải
    lv_obj_t *sig_cont = create_signal_widget(top_bar, &h_bar_sig_1, &h_bar_sig_2, &h_bar_sig_3, &h_bar_sig_4, &h_lbl_sig_x);
    lv_obj_align(sig_cont, LV_ALIGN_RIGHT_MID, -12, 0);

    // 2. GIANT DESK CLOCK (Montserrat 40)
    lbl_home_big_clock = lv_label_create(scr_home);
    lv_label_set_text(lbl_home_big_clock, "--:--:--");
    lv_obj_set_style_text_color(lbl_home_big_clock, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_home_big_clock, &lv_font_montserrat_40, 0);
    lv_obj_align(lbl_home_big_clock, LV_ALIGN_CENTER, 0, -32);

    // 3. CALENDAR SUBTITLE
    lbl_home_calendar = lv_label_create(scr_home);
    lv_label_set_text(lbl_home_calendar, "AI Developer Desk Console");
    lv_obj_set_style_text_color(lbl_home_calendar, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(lbl_home_calendar, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_home_calendar, LV_ALIGN_CENTER, 0, 10);

    // 4. MINI QUOTA WIDGET (Panel mini bo góc ở đáy)
    lv_obj_t *panel_mini = lv_obj_create(scr_home);
    lv_obj_set_size(panel_mini, 290, 48);
    lv_obj_set_style_bg_color(panel_mini, lv_color_hex(0x162032), 0);
    lv_obj_set_style_bg_opa(panel_mini, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel_mini, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(panel_mini, 1, 0);
    lv_obj_set_style_radius(panel_mini, 8, 0);
    lv_obj_set_style_pad_all(panel_mini, 0, 0);
    lv_obj_align(panel_mini, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_clear_flag(panel_mini, LV_OBJ_FLAG_SCROLLABLE);

    // Gemini Mini
    lv_obj_t *lbl_mg = lv_label_create(panel_mini);
    lv_label_set_text(lbl_mg, "GEMINI");
    lv_obj_set_style_text_color(lbl_mg, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_mg, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_mg, 10, 6);

    bar_home_gemini = lv_bar_create(panel_mini);
    lv_obj_set_size(bar_home_gemini, 85, 8);
    lv_bar_set_range(bar_home_gemini, 0, 100);
    lv_bar_set_value(bar_home_gemini, 90, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_home_gemini, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_home_gemini, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_home_gemini, 3, 0);
    lv_obj_set_pos(bar_home_gemini, 10, 26);

    lbl_home_gem_val = lv_label_create(panel_mini);
    lv_label_set_text(lbl_home_gem_val, "90%");
    lv_obj_set_style_text_color(lbl_home_gem_val, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_home_gem_val, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_home_gem_val, 102, 22);

    // Vách ngăn giữa
    lv_obj_t *sep = lv_obj_create(panel_mini);
    lv_obj_set_size(sep, 1, 32);
    lv_obj_set_pos(sep, 145, 8);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(sep, 0, 0);

    // Claude Mini
    lv_obj_t *lbl_mc = lv_label_create(panel_mini);
    lv_label_set_text(lbl_mc, "CLAUDE");
    lv_obj_set_style_text_color(lbl_mc, lv_color_hex(0xF97316), 0);
    lv_obj_set_style_text_font(lbl_mc, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_mc, 155, 6);

    bar_home_claude = lv_bar_create(panel_mini);
    lv_obj_set_size(bar_home_claude, 85, 8);
    lv_bar_set_range(bar_home_claude, 0, 100);
    lv_bar_set_value(bar_home_claude, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_home_claude, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_home_claude, lv_color_hex(0xF97316), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_home_claude, 3, 0);
    lv_obj_set_pos(bar_home_claude, 155, 26);

    lbl_home_cld_val = lv_label_create(panel_mini);
    lv_label_set_text(lbl_home_cld_val, "100%");
    lv_obj_set_style_text_color(lbl_home_cld_val, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_home_cld_val, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl_home_cld_val, 247, 22);
}

// =========================================================================
// Real-Time Quota & Telemetry Data Update Function
// =========================================================================
esp_err_t display_ui_update_quota(const pc_quota_packet_t *quota) {
    if (!quota) return ESP_ERR_INVALID_ARG;

    if (lvgl_lock(100)) {
        char buf[64];

        // 1. Đồng hồ (Cập nhật cho cả 2 màn hình)
        if (quota->time_vn[0] != '\0') {
            if (lbl_clock) lv_label_set_text(lbl_clock, quota->time_vn);
            if (lbl_home_big_clock) lv_label_set_text(lbl_home_big_clock, quota->time_vn);
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

    // 10. Build Screens (Console + Home)
    if (lvgl_lock(-1)) {
        build_console_screen();
        build_home_screen();
        lv_scr_load(scr_console); // Mặc định mở màn hình Dashboard Quota
        set_signal_level(0);       // Khởi động chưa có BLE: hiện dấu x đỏ

        // 11. Cảm ứng XPT2046
        touch_xpt2046_init();
        touch_xpt2046_register_lvgl();

        lvgl_unlock();
    }

    ESP_LOGI(TAG, "Multi-Screen Dashboard UI initialized successfully!");
    return ESP_OK;
}
