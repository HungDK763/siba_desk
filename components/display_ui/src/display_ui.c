/*
 * AI Developer Desk Console & Universal Gateway
 * Display UI Component (Landscape 320x240, Header on LEFT)
 * Hardware Init: 100% verified configuration (MADCTL = 0xA0)
 */

#include <stdio.h>
#include <string.h>
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

// UI Widgets pointers for live data updates
static lv_obj_t *lbl_clock = NULL;
static lv_obj_t *lbl_ble = NULL;
static lv_obj_t *lbl_model = NULL;

static lv_obj_t *arc_gemini = NULL;
static lv_obj_t *lbl_gemini_val = NULL;
static lv_obj_t *bar_gemini_w = NULL;

static lv_obj_t *arc_claude = NULL;
static lv_obj_t *lbl_claude_val = NULL;
static lv_obj_t *bar_claude_w = NULL;

static lv_obj_t *lbl_prompts = NULL;
static lv_obj_t *lbl_system = NULL;

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

static void create_dashboard_ui(lv_disp_t *disp) {
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_clean(scr);

    // Deep Slate Background (#0F172A)
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // ==========================================
    // 1. TOP BAR (Height: 26px)
    // ==========================================
    lv_obj_t *top_bar = lv_obj_create(scr);
    lv_obj_set_pos(top_bar, 0, 0);
    lv_obj_set_size(top_bar, 320, 26);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    lbl_clock = lv_label_create(top_bar);
    lv_label_set_text(lbl_clock, "--:--:--");
    lv_obj_set_style_text_color(lbl_clock, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_clock, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_clock, LV_ALIGN_LEFT_MID, 8, 0);

    lv_obj_t *lbl_title = lv_label_create(top_bar);
    lv_label_set_text(lbl_title, "AI CONSOLE");
    lv_obj_set_style_text_color(lbl_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);

    lbl_ble = lv_label_create(top_bar);
    lv_label_set_text(lbl_ble, "● BLE: READY");
    lv_obj_set_style_text_color(lbl_ble, lv_color_hex(0x22C55E), 0);
    lv_obj_align(lbl_ble, LV_ALIGN_RIGHT_MID, -8, 0);

    // ==========================================
    // 2. MODEL SUBTITLE (Height: 18px, Y = 28)
    // ==========================================
    lbl_model = lv_label_create(scr);
    lv_label_set_text(lbl_model, "Model: Gemini 3.8 Flash (High)");
    lv_obj_set_style_text_color(lbl_model, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(lbl_model, LV_ALIGN_TOP_MID, 0, 30);

    // ==========================================
    // 3. CARD GEMINI (Left: X = 8, Y = 50, W = 148, H = 142)
    // ==========================================
    lv_obj_t *card_gemini = lv_obj_create(scr);
    lv_obj_set_pos(card_gemini, 8, 50);
    lv_obj_set_size(card_gemini, 148, 142);
    lv_obj_set_style_bg_color(card_gemini, lv_color_hex(0x162032), 0);
    lv_obj_set_style_bg_opa(card_gemini, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card_gemini, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(card_gemini, 1, 0);
    lv_obj_set_style_radius(card_gemini, 8, 0);
    lv_obj_set_style_pad_all(card_gemini, 0, 0);
    lv_obj_clear_flag(card_gemini, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_gem_title = lv_label_create(card_gemini);
    lv_label_set_text(lbl_gem_title, "GEMINI");
    lv_obj_set_style_text_color(lbl_gem_title, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_gem_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_gem_title, LV_ALIGN_TOP_MID, 0, 4);

    arc_gemini = lv_arc_create(card_gemini);
    lv_obj_set_size(arc_gemini, 78, 78);
    lv_arc_set_rotation(arc_gemini, 135);
    lv_arc_set_bg_angles(arc_gemini, 0, 270);
    lv_arc_set_value(arc_gemini, 90);
    lv_obj_set_style_arc_color(arc_gemini, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_gemini, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_gemini, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_gemini, 6, LV_PART_INDICATOR);
    lv_obj_remove_style(arc_gemini, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_gemini, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(arc_gemini, LV_ALIGN_TOP_MID, 0, 22);

    lbl_gemini_val = lv_label_create(arc_gemini);
    lv_label_set_text(lbl_gemini_val, "90%");
    lv_obj_set_style_text_color(lbl_gemini_val, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_gemini_val, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_gemini_val);

    lv_obj_t *lbl_gem_w = lv_label_create(card_gemini);
    lv_label_set_text(lbl_gem_w, "Weekly");
    lv_obj_set_style_text_color(lbl_gem_w, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(lbl_gem_w, LV_ALIGN_BOTTOM_LEFT, 8, -6);

    bar_gemini_w = lv_bar_create(card_gemini);
    lv_obj_set_size(bar_gemini_w, 75, 6);
    lv_bar_set_range(bar_gemini_w, 0, 100);
    lv_bar_set_value(bar_gemini_w, 71, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_gemini_w, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_gemini_w, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
    lv_obj_align(bar_gemini_w, LV_ALIGN_BOTTOM_RIGHT, -8, -8);

    // ==========================================
    // 4. CARD CLAUDE (Right: X = 164, Y = 50, W = 148, H = 142)
    // ==========================================
    lv_obj_t *card_claude = lv_obj_create(scr);
    lv_obj_set_pos(card_claude, 164, 50);
    lv_obj_set_size(card_claude, 148, 142);
    lv_obj_set_style_bg_color(card_claude, lv_color_hex(0x162032), 0);
    lv_obj_set_style_bg_opa(card_claude, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card_claude, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(card_claude, 1, 0);
    lv_obj_set_style_radius(card_claude, 8, 0);
    lv_obj_set_style_pad_all(card_claude, 0, 0);
    lv_obj_clear_flag(card_claude, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_cld_title = lv_label_create(card_claude);
    lv_label_set_text(lbl_cld_title, "CLAUDE");
    lv_obj_set_style_text_color(lbl_cld_title, lv_color_hex(0xF97316), 0);
    lv_obj_set_style_text_font(lbl_cld_title, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_cld_title, LV_ALIGN_TOP_MID, 0, 4);

    arc_claude = lv_arc_create(card_claude);
    lv_obj_set_size(arc_claude, 78, 78);
    lv_arc_set_rotation(arc_claude, 135);
    lv_arc_set_bg_angles(arc_claude, 0, 270);
    lv_arc_set_value(arc_claude, 100);
    lv_obj_set_style_arc_color(arc_claude, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_claude, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_claude, lv_color_hex(0xF97316), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_claude, 6, LV_PART_INDICATOR);
    lv_obj_remove_style(arc_claude, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_claude, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(arc_claude, LV_ALIGN_TOP_MID, 0, 22);

    lbl_claude_val = lv_label_create(arc_claude);
    lv_label_set_text(lbl_claude_val, "100%");
    lv_obj_set_style_text_color(lbl_claude_val, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_claude_val, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_claude_val);

    lv_obj_t *lbl_cld_w = lv_label_create(card_claude);
    lv_label_set_text(lbl_cld_w, "Weekly");
    lv_obj_set_style_text_color(lbl_cld_w, lv_color_hex(0x94A3B8), 0);
    lv_obj_align(lbl_cld_w, LV_ALIGN_BOTTOM_LEFT, 8, -6);

    bar_claude_w = lv_bar_create(card_claude);
    lv_obj_set_size(bar_claude_w, 75, 6);
    lv_bar_set_range(bar_claude_w, 0, 100);
    lv_bar_set_value(bar_claude_w, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_claude_w, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_claude_w, lv_color_hex(0xF97316), LV_PART_INDICATOR);
    lv_obj_align(bar_claude_w, LV_ALIGN_BOTTOM_RIGHT, -8, -8);

    // ==========================================
    // 5. BOTTOM BAR (Height: 24px, Y = 216)
    // ==========================================
    lv_obj_t *bottom_bar = lv_obj_create(scr);
    lv_obj_set_pos(bottom_bar, 0, 216);
    lv_obj_set_size(bottom_bar, 320, 24);
    lv_obj_set_style_bg_color(bottom_bar, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bottom_bar, 0, 0);
    lv_obj_set_style_border_width(bottom_bar, 0, 0);
    lv_obj_set_style_pad_all(bottom_bar, 0, 0);
    lv_obj_clear_flag(bottom_bar, LV_OBJ_FLAG_SCROLLABLE);

    lbl_prompts = lv_label_create(bottom_bar);
    lv_label_set_text(lbl_prompts, "Prompts: 121");
    lv_obj_set_style_text_color(lbl_prompts, lv_color_hex(0xCBD5E1), 0);
    lv_obj_align(lbl_prompts, LV_ALIGN_LEFT_MID, 8, 0);

    lbl_system = lv_label_create(bottom_bar);
    lv_label_set_text(lbl_system, "CPU: 36% | RAM: 41%");
    lv_obj_set_style_text_color(lbl_system, lv_color_hex(0xCBD5E1), 0);
    lv_obj_align(lbl_system, LV_ALIGN_RIGHT_MID, -8, 0);
}

esp_err_t display_ui_update_quota(const pc_quota_packet_t *quota) {
    if (!quota) return ESP_ERR_INVALID_ARG;

    if (lvgl_lock(100)) {
        char buf[64];

        // 1. Update Clock
        if (lbl_clock && quota->time_vn[0] != '\0') {
            lv_label_set_text(lbl_clock, quota->time_vn);
        }

        // 2. Update Model Name
        if (lbl_model && quota->model_name[0] != '\0') {
            snprintf(buf, sizeof(buf), "Model: %s", quota->model_name);
            lv_label_set_text(lbl_model, buf);
        }

        // 3. Update Gemini Quota
        if (arc_gemini && lbl_gemini_val) {
            lv_arc_set_value(arc_gemini, quota->antigrav_quota_percent);
            snprintf(buf, sizeof(buf), "%d%%", quota->antigrav_quota_percent);
            lv_label_set_text(lbl_gemini_val, buf);

            // Change arc color to red/amber if quota low
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

        // 4. Update Claude Quota
        if (arc_claude && lbl_claude_val) {
            lv_arc_set_value(arc_claude, quota->claude_5h_percent);
            snprintf(buf, sizeof(buf), "%d%%", quota->claude_5h_percent);
            lv_label_set_text(lbl_claude_val, buf);
        }
        if (bar_claude_w) {
            lv_bar_set_value(bar_claude_w, quota->claude_weekly_percent, LV_ANIM_OFF);
        }

        // 5. Update Prompts
        if (lbl_prompts) {
            snprintf(buf, sizeof(buf), "Prompts: %lu", (unsigned long)quota->prompt_count);
            lv_label_set_text(lbl_prompts, buf);
        }

        // 6. Update Host PC CPU / RAM Telemetry
        if (lbl_system) {
            snprintf(buf, sizeof(buf), "CPU: %d%% | RAM: %d%%", quota->cpu_percent, quota->ram_percent);
            lv_label_set_text(lbl_system, buf);
        }

        // 7. Update BLE Status indicator
        if (lbl_ble) {
            lv_label_set_text(lbl_ble, "● BLE: SYNCED");
            lv_obj_set_style_text_color(lbl_ble, lv_color_hex(0x22C55E), 0);
        }

        lvgl_unlock();
    }

    return ESP_OK;
}

esp_err_t display_ui_init(void) {
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "  AI CONSOLE DASHBOARD (EXACT TEST CONFIG)       ");
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

    // 6. EXACT HARDWARE INIT (Verified 1:1 with Raw SPI test)
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

    // 10. Create Dashboard UI
    if (lvgl_lock(-1)) {
        create_dashboard_ui(disp_handle);
        lvgl_unlock();
    }

    ESP_LOGI(TAG, "AI Console Dashboard UI initialized successfully!");
    return ESP_OK;
}
