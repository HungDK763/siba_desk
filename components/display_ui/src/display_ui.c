#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "display_ui.h"

static const char *TAG = "DISP_UI";

// Định nghĩa chân phần cứng thẳng hàng với Module TFT theo docs_for_me/GPIO_Define.md
#define LCD_HOST               SPI2_HOST
#define LCD_PIN_CS             GPIO_NUM_14
#define LCD_PIN_RST            GPIO_NUM_13
#define LCD_PIN_DC             GPIO_NUM_12
#define LCD_PIN_MOSI           GPIO_NUM_11
#define LCD_PIN_CLK            GPIO_NUM_10
#define LCD_PIN_BL             GPIO_NUM_9
#define LCD_PIN_MISO           GPIO_NUM_46

#define LCD_H_RES              320
#define LCD_V_RES              240

#define LCD_BL_LEDC_TIMER      LEDC_TIMER_0
#define LCD_BL_LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LCD_BL_LEDC_CHANNEL    LEDC_CHANNEL_0
#define LCD_BL_LEDC_DUTY_RES   LEDC_TIMER_8_BIT
#define LCD_BL_LEDC_FREQ_HZ    5000

#if defined(LV_FONT_MONTSERRAT_10) && LV_FONT_MONTSERRAT_10
#define FONT_SMALL &lv_font_montserrat_10
#else
#define FONT_SMALL LV_FONT_DEFAULT
#endif

#if defined(LV_FONT_MONTSERRAT_12) && LV_FONT_MONTSERRAT_12
#define FONT_MEDIUM &lv_font_montserrat_12
#else
#define FONT_MEDIUM LV_FONT_DEFAULT
#endif

#if defined(LV_FONT_MONTSERRAT_14) && LV_FONT_MONTSERRAT_14
#define FONT_LARGE &lv_font_montserrat_14
#else
#define FONT_LARGE LV_FONT_DEFAULT
#endif

static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_disp_t *lvgl_disp = NULL;

// Các đối tượng widget trên Dashboard LVGL
static lv_obj_t *lbl_clock = NULL;
static lv_obj_t *lbl_model = NULL;
static lv_obj_t *lbl_ble_status = NULL;

static lv_obj_t *arc_gemini = NULL;
static lv_obj_t *lbl_gemini_val = NULL;
static lv_obj_t *bar_gemini_w = NULL;
static lv_obj_t *lbl_gemini_w_val = NULL;

static lv_obj_t *arc_claude = NULL;
static lv_obj_t *lbl_claude_val = NULL;
static lv_obj_t *bar_claude_w = NULL;
static lv_obj_t *lbl_claude_w_val = NULL;

static lv_obj_t *lbl_prompts = NULL;
static lv_obj_t *lbl_system = NULL;

static esp_err_t backlight_init(void) {
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LCD_BL_LEDC_DUTY_RES,
        .freq_hz = LCD_BL_LEDC_FREQ_HZ,
        .speed_mode = LCD_BL_LEDC_MODE,
        .timer_num = LCD_BL_LEDC_TIMER,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .channel    = LCD_BL_LEDC_CHANNEL,
        .duty       = 255, // Mặc định 100% độ sáng
        .gpio_num   = LCD_PIN_BL,
        .speed_mode = LCD_BL_LEDC_MODE,
        .hpoint     = 0,
        .timer_sel  = LCD_BL_LEDC_TIMER
    };
    return ledc_channel_config(&ledc_channel);
}

esp_err_t display_ui_set_backlight(uint8_t brightness_pct) {
    if (brightness_pct > 100) brightness_pct = 100;
    uint32_t duty = (brightness_pct * 255) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL, duty));
    return ledc_update_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL);
}

static void create_dashboard_ui(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0F172A), 0); // Slate 900 dark background

    // ==========================================
    // 1. TOP STATUS BAR (H: 26px)
    // ==========================================
    lv_obj_t *top_bar = lv_obj_create(scr);
    lv_obj_set_size(top_bar, 320, 28);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 4, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_title = lv_label_create(top_bar);
    lv_label_set_text(lbl_title, "AI CONSOLE");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x38BDF8), 0); // Sky 400
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 6, 0);

    lbl_clock = lv_label_create(top_bar);
    lv_label_set_text(lbl_clock, "--:--:--");
    lv_obj_set_style_text_color(lbl_clock, lv_color_hex(0xF8FAFC), 0); // White
    lv_obj_align(lbl_clock, LV_ALIGN_CENTER, 0, 0);

    lbl_ble_status = lv_label_create(top_bar);
    lv_label_set_text(lbl_ble_status, "BLE: OK");
    lv_obj_set_style_text_color(lbl_ble_status, lv_color_hex(0x22C55E), 0); // Green 500
    lv_obj_align(lbl_ble_status, LV_ALIGN_RIGHT_MID, -6, 0);

    // ==========================================
    // 2. MODEL BANNER (H: 20px)
    // ==========================================
    lbl_model = lv_label_create(scr);
    lv_label_set_text(lbl_model, "Model: Gemini 3.8 Flash (High)");
    lv_obj_set_style_text_color(lbl_model, lv_color_hex(0x94A3B8), 0); // Slate 400
    lv_obj_set_style_text_font(lbl_model, FONT_SMALL, 0);
    lv_obj_align(lbl_model, LV_ALIGN_TOP_MID, 0, 32);

    // ==========================================
    // 3. CARD 1: GEMINI QUOTA (Left: x=8, y=52, w=148, h=146)
    // ==========================================
    lv_obj_t *card_gemini = lv_obj_create(scr);
    lv_obj_set_size(card_gemini, 148, 146);
    lv_obj_align(card_gemini, LV_ALIGN_TOP_LEFT, 8, 52);
    lv_obj_set_style_bg_color(card_gemini, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_color(card_gemini, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(card_gemini, 1, 0);
    lv_obj_set_style_radius(card_gemini, 8, 0);
    lv_obj_set_style_pad_all(card_gemini, 4, 0);
    lv_obj_clear_flag(card_gemini, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_gem_title = lv_label_create(card_gemini);
    lv_label_set_text(lbl_gem_title, "GEMINI 5H");
    lv_obj_set_style_text_color(lbl_gem_title, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(lbl_gem_title, FONT_MEDIUM, 0);
    lv_obj_align(lbl_gem_title, LV_ALIGN_TOP_MID, 0, 2);

    arc_gemini = lv_arc_create(card_gemini);
    lv_obj_set_size(arc_gemini, 80, 80);
    lv_arc_set_rotation(arc_gemini, 135);
    lv_arc_set_bg_angles(arc_gemini, 0, 270);
    lv_arc_set_value(arc_gemini, 100);
    lv_obj_align(arc_gemini, LV_ALIGN_TOP_MID, 0, 18);
    lv_obj_remove_style(arc_gemini, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_gemini, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_gemini, 7, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_gemini, 7, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_gemini, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_gemini, lv_color_hex(0x22C55E), LV_PART_INDICATOR);

    lbl_gemini_val = lv_label_create(card_gemini);
    lv_label_set_text(lbl_gemini_val, "100%");
    lv_obj_set_style_text_color(lbl_gemini_val, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_text_font(lbl_gemini_val, FONT_LARGE, 0);
    lv_obj_align(lbl_gemini_val, LV_ALIGN_TOP_MID, 0, 48);

    lbl_gemini_w_val = lv_label_create(card_gemini);
    lv_label_set_text(lbl_gemini_w_val, "Weekly: 85%");
    lv_obj_set_style_text_color(lbl_gemini_w_val, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(lbl_gemini_w_val, FONT_SMALL, 0);
    lv_obj_align(lbl_gemini_w_val, LV_ALIGN_BOTTOM_MID, 0, -16);

    bar_gemini_w = lv_bar_create(card_gemini);
    lv_obj_set_size(bar_gemini_w, 130, 8);
    lv_obj_align(bar_gemini_w, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_bar_set_value(bar_gemini_w, 85, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_gemini_w, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_gemini_w, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);

    // ==========================================
    // 4. CARD 2: CLAUDE & GPT QUOTA (Right: x=164, y=52, w=148, h=146)
    // ==========================================
    lv_obj_t *card_claude = lv_obj_create(scr);
    lv_obj_set_size(card_claude, 148, 146);
    lv_obj_align(card_claude, LV_ALIGN_TOP_RIGHT, -8, 52);
    lv_obj_set_style_bg_color(card_claude, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_border_color(card_claude, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(card_claude, 1, 0);
    lv_obj_set_style_radius(card_claude, 8, 0);
    lv_obj_set_style_pad_all(card_claude, 4, 0);
    lv_obj_clear_flag(card_claude, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_cld_title = lv_label_create(card_claude);
    lv_label_set_text(lbl_cld_title, "CLAUDE 5H");
    lv_obj_set_style_text_color(lbl_cld_title, lv_color_hex(0xA855F7), 0); // Purple 500
    lv_obj_set_style_text_font(lbl_cld_title, FONT_MEDIUM, 0);
    lv_obj_align(lbl_cld_title, LV_ALIGN_TOP_MID, 0, 2);

    arc_claude = lv_arc_create(card_claude);
    lv_obj_set_size(arc_claude, 80, 80);
    lv_arc_set_rotation(arc_claude, 135);
    lv_arc_set_bg_angles(arc_claude, 0, 270);
    lv_arc_set_value(arc_claude, 100);
    lv_obj_align(arc_claude, LV_ALIGN_TOP_MID, 0, 18);
    lv_obj_remove_style(arc_claude, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_claude, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_claude, 7, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_claude, 7, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_claude, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_claude, lv_color_hex(0x22C55E), LV_PART_INDICATOR);

    lbl_claude_val = lv_label_create(card_claude);
    lv_label_set_text(lbl_claude_val, "100%");
    lv_obj_set_style_text_color(lbl_claude_val, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_text_font(lbl_claude_val, FONT_LARGE, 0);
    lv_obj_align(lbl_claude_val, LV_ALIGN_TOP_MID, 0, 48);

    lbl_claude_w_val = lv_label_create(card_claude);
    lv_label_set_text(lbl_claude_w_val, "Weekly: 100%");
    lv_obj_set_style_text_color(lbl_claude_w_val, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(lbl_claude_w_val, FONT_SMALL, 0);
    lv_obj_align(lbl_claude_w_val, LV_ALIGN_BOTTOM_MID, 0, -16);

    bar_claude_w = lv_bar_create(card_claude);
    lv_obj_set_size(bar_claude_w, 130, 8);
    lv_obj_align(bar_claude_w, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_bar_set_value(bar_claude_w, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_claude_w, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_claude_w, lv_color_hex(0xA855F7), LV_PART_INDICATOR);

    // ==========================================
    // 5. BOTTOM BAR: TELEMETRY (H: 26px)
    // ==========================================
    lv_obj_t *bot_bar = lv_obj_create(scr);
    lv_obj_set_size(bot_bar, 320, 26);
    lv_obj_align(bot_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bot_bar, lv_color_hex(0x020617), 0);
    lv_obj_set_style_border_width(bot_bar, 0, 0);
    lv_obj_set_style_radius(bot_bar, 0, 0);
    lv_obj_set_style_pad_all(bot_bar, 3, 0);
    lv_obj_clear_flag(bot_bar, LV_OBJ_FLAG_SCROLLABLE);

    lbl_prompts = lv_label_create(bot_bar);
    lv_label_set_text(lbl_prompts, "Prompts: 0");
    lv_obj_set_style_text_color(lbl_prompts, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_text_font(lbl_prompts, FONT_SMALL, 0);
    lv_obj_align(lbl_prompts, LV_ALIGN_LEFT_MID, 8, 0);

    lbl_system = lv_label_create(bot_bar);
    lv_label_set_text(lbl_system, "CPU: 0% | RAM: 0%");
    lv_obj_set_style_text_color(lbl_system, lv_color_hex(0xCBD5E1), 0);
    lv_obj_set_style_text_font(lbl_system, FONT_SMALL, 0);
    lv_obj_align(lbl_system, LV_ALIGN_RIGHT_MID, -8, 0);
}

esp_err_t display_ui_init(void) {
    ESP_LOGI(TAG, "Initializing LCD Hardware and LVGL Port...");

    // 1. Khởi tạo đèn nền LCD
    backlight_init();
    display_ui_set_backlight(100);

    // 2. Cấu hình SPI Bus
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_PIN_CLK,
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = LCD_PIN_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 40 * sizeof(uint16_t),
    };
    esp_err_t ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI Bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // 3. Cấu hình Panel IO
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_DC,
        .cs_gpio_num = LCD_PIN_CS,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create panel IO: %s", esp_err_to_name(ret));
        return ret;
    }

    // 4. Khởi tạo driver panel ILI9341
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ret = esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ILI9341 panel: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true); // Sửa màu âm bản (chuyển nền trắng thành nền đen Slate)
    esp_lcd_panel_swap_xy(panel_handle, true);      // Xoay ngang màn hình (Landscape 320x240)
    esp_lcd_panel_mirror(panel_handle, true, false);// Sửa chữ bị lật ngược gương
    esp_lcd_panel_disp_on_off(panel_handle, true);

    // 5. Khởi tạo LVGL Port Task
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_stack = 8192;
    port_cfg.task_priority = 4;
    ret = lvgl_port_init(&port_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL port: %s", esp_err_to_name(ret));
        return ret;
    }

    // 6. Đăng ký màn hình vào LVGL
    lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .control_handle = panel_handle,
        .buffer_size = LCD_H_RES * 20, // 20 dòng buffer
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = true,  // Màn hình ngang (Landscape 320x240)
            .mirror_x = true, // Đảo mirror_x để chữ đọc xuôi từ trái qua phải
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    if (!lvgl_disp) {
        ESP_LOGE(TAG, "Failed to add display to LVGL");
        return ESP_FAIL;
    }

    // 7. Tạo giao diện đồ họa Dashboard
    if (lvgl_port_lock(0)) {
        create_dashboard_ui();
        lvgl_port_unlock();
    }

    ESP_LOGI(TAG, "✨ LVGL v8 Display UI initialized successfully (320x240 Landscape)");
    return ESP_OK;
}

esp_err_t display_ui_update_quota(const pc_quota_packet_t *quota) {
    if (!quota) return ESP_ERR_INVALID_ARG;
    if (!lvgl_disp) return ESP_ERR_INVALID_STATE;

    if (lvgl_port_lock(0)) {
        // Cập nhật Giờ Việt Nam
        if (lbl_clock && quota->time_vn[0] != '\0') {
            lv_label_set_text(lbl_clock, quota->time_vn);
        }

        // Cập nhật Model AI
        if (lbl_model && quota->model_name[0] != '\0') {
            char buf[64];
            snprintf(buf, sizeof(buf), "Model: %s", quota->model_name);
            lv_label_set_text(lbl_model, buf);
        }

        // Cập nhật Gemini 5h Quota
        if (arc_gemini) {
            lv_arc_set_value(arc_gemini, quota->antigrav_quota_percent);
            lv_color_t gem_col = lv_color_hex(0x22C55E); // Green
            if (quota->alert_level == QUOTA_ALERT_CRITICAL) {
                gem_col = lv_color_hex(0xEF4444); // Red
            } else if (quota->alert_level == QUOTA_ALERT_WARNING) {
                gem_col = lv_color_hex(0xEAB308); // Yellow
            }
            lv_obj_set_style_arc_color(arc_gemini, gem_col, LV_PART_INDICATOR);
        }
        if (lbl_gemini_val) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d%%", quota->antigrav_quota_percent);
            lv_label_set_text(lbl_gemini_val, buf);
        }

        // Cập nhật Gemini Weekly Quota
        if (bar_gemini_w) {
            lv_bar_set_value(bar_gemini_w, quota->gemini_weekly_percent, LV_ANIM_ON);
        }
        if (lbl_gemini_w_val) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Weekly: %d%%", quota->gemini_weekly_percent);
            lv_label_set_text(lbl_gemini_w_val, buf);
        }

        // Cập nhật Claude 5h Quota
        if (arc_claude) {
            lv_arc_set_value(arc_claude, quota->claude_5h_percent);
            lv_color_t cld_col = lv_color_hex(0x22C55E);
            if (quota->claude_5h_percent < 20) {
                cld_col = lv_color_hex(0xEF4444);
            } else if (quota->claude_5h_percent < 50) {
                cld_col = lv_color_hex(0xEAB308);
            }
            lv_obj_set_style_arc_color(arc_claude, cld_col, LV_PART_INDICATOR);
        }
        if (lbl_claude_val) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d%%", quota->claude_5h_percent);
            lv_label_set_text(lbl_claude_val, buf);
        }

        // Cập nhật Claude Weekly Quota
        if (bar_claude_w) {
            lv_bar_set_value(bar_claude_w, quota->claude_weekly_percent, LV_ANIM_ON);
        }
        if (lbl_claude_w_val) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Weekly: %d%%", quota->claude_weekly_percent);
            lv_label_set_text(lbl_claude_w_val, buf);
        }

        // Cập nhật Prompts
        if (lbl_prompts) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Prompts: %lu", (unsigned long)quota->prompt_count);
            lv_label_set_text(lbl_prompts, buf);
        }

        // Cập nhật CPU & RAM
        if (lbl_system) {
            char buf[32];
            snprintf(buf, sizeof(buf), "CPU: %d%% | RAM: %d%%", quota->cpu_percent, quota->ram_percent);
            lv_label_set_text(lbl_system, buf);
        }

        lvgl_port_unlock();
    }

    return ESP_OK;
}
