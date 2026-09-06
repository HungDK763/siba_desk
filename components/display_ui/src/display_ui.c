/*
 * Hardware SPI Test Step 2:
 * 1. Fix Red/Blue swap: Set MADCTL BGR bit = 0 (RGB mode)
 * 2. Set Landscape 320x240 (Header on RIGHT: MADCTL = 0xE0)
 * 3. Draw 4 Quadrants to verify orientation:
 *    - TOP-LEFT:     RED
 *    - TOP-RIGHT:    GREEN
 *    - BOTTOM-LEFT:  BLUE
 *    - BOTTOM-RIGHT: WHITE
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"

#include "display_ui.h"

static const char *TAG = "HW_DIAG";

#define LCD_HOST                SPI2_HOST
#define PIN_NUM_SCLK            GPIO_NUM_10
#define PIN_NUM_MOSI            GPIO_NUM_11
#define PIN_NUM_MISO            GPIO_NUM_46
#define PIN_NUM_DC              GPIO_NUM_12
#define PIN_NUM_RST             GPIO_NUM_13
#define PIN_NUM_CS              GPIO_NUM_14
#define PIN_NUM_BL              GPIO_NUM_9

#define H_RES                   320
#define V_RES                   240

static spi_device_handle_t spi_dev = NULL;

esp_err_t display_ui_set_backlight(uint8_t brightness_pct) {
    gpio_set_level(PIN_NUM_BL, brightness_pct > 0 ? 1 : 0);
    return ESP_OK;
}

esp_err_t display_ui_update_quota(const pc_quota_packet_t *quota) {
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
    send_cmd(0x2A); // CASET (Column: 0..319)
    uint8_t caset[] = { (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF), (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF) };
    send_data(caset, 4);

    send_cmd(0x2B); // RASET (Row: 0..239)
    uint8_t raset[] = { (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF), (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF) };
    send_data(raset, 4);

    send_cmd(0x2C); // RAMWR
}

#define SWAP_BYTES(v) ((uint16_t)(((uint16_t)(v) >> 8) | (((uint16_t)(v) & 0xFF) << 8)))

static void fill_solid_color(uint16_t color_rgb565) {
    set_window(0, 0, H_RES - 1, V_RES - 1);

    static uint16_t line_buf[H_RES];
    uint16_t be_color = SWAP_BYTES(color_rgb565);
    for (int i = 0; i < H_RES; i++) {
        line_buf[i] = be_color;
    }

    gpio_set_level(PIN_NUM_DC, 1);
    for (int y = 0; y < V_RES; y++) {
        spi_transaction_t t = {
            .length = H_RES * 16,
            .tx_buffer = line_buf,
        };
        spi_device_polling_transmit(spi_dev, &t);
    }
}

static void draw_4_quadrants(void) {
    set_window(0, 0, H_RES - 1, V_RES - 1);

    static uint16_t line_buf[H_RES];
    uint16_t c_red   = SWAP_BYTES(0xF800);
    uint16_t c_green = SWAP_BYTES(0x07E0);
    uint16_t c_blue  = SWAP_BYTES(0x001F);
    uint16_t c_white = SWAP_BYTES(0xFFFF);
    uint16_t c_black = 0x0000;

    gpio_set_level(PIN_NUM_DC, 1);
    for (int y = 0; y < V_RES; y++) {
        for (int x = 0; x < H_RES; x++) {
            // 3px black outer border
            if (x < 3 || x >= H_RES - 3 || y < 3 || y >= V_RES - 3) {
                line_buf[x] = c_black;
            }
            // 2px center crosshair
            else if (x == 159 || x == 160 || y == 119 || y == 120) {
                line_buf[x] = c_black;
            }
            // Top-Left: RED
            else if (x < 160 && y < 120) {
                line_buf[x] = c_red;
            }
            // Top-Right: GREEN
            else if (x >= 160 && y < 120) {
                line_buf[x] = c_green;
            }
            // Bottom-Left: BLUE
            else if (x < 160 && y >= 120) {
                line_buf[x] = c_blue;
            }
            // Bottom-Right: WHITE
            else {
                line_buf[x] = c_white;
            }
        }
        spi_transaction_t t = {
            .length = H_RES * 16,
            .tx_buffer = line_buf,
        };
        spi_device_polling_transmit(spi_dev, &t);
    }
}

static void diag_task(void *arg) {
    vTaskDelay(pdMS_TO_TICKS(500));

    while (1) {
        ESP_LOGI(TAG, "--------------------------------------------------------");
        ESP_LOGI(TAG, "  🔴 DANG XUAT: DO THUAN (RED - 0xF800) | Full 320x240  ");
        ESP_LOGI(TAG, "--------------------------------------------------------");
        fill_solid_color(0xF800);
        vTaskDelay(pdMS_TO_TICKS(3000));

        ESP_LOGI(TAG, "--------------------------------------------------------");
        ESP_LOGI(TAG, "  🟢 DANG XUAT: XANH LA (GREEN - 0x07E0) | Full 320x240 ");
        ESP_LOGI(TAG, "--------------------------------------------------------");
        fill_solid_color(0x07E0);
        vTaskDelay(pdMS_TO_TICKS(3000));

        ESP_LOGI(TAG, "--------------------------------------------------------");
        ESP_LOGI(TAG, "  🔵 DANG XUAT: XANH DUONG (BLUE - 0x001F) | Full 320x240");
        ESP_LOGI(TAG, "--------------------------------------------------------");
        fill_solid_color(0x001F);
        vTaskDelay(pdMS_TO_TICKS(3000));

        ESP_LOGI(TAG, "--------------------------------------------------------");
        ESP_LOGI(TAG, "  🎯 DANG XUAT: 4 O GOC LANDSCAPE (320x240)             ");
        ESP_LOGI(TAG, "     TREN-TRAI : 🔴 DO        | TREN-PHAI : 🟢 XANH LA ");
        ESP_LOGI(TAG, "     DUOI-TRAI : 🔵 XANH DUONG| DUOI-PHAI : ⚪ TRANG   ");
        ESP_LOGI(TAG, "--------------------------------------------------------");
        draw_4_quadrants();
        vTaskDelay(pdMS_TO_TICKS(6000));
    }
}

esp_err_t display_ui_init(void) {
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "   STEP 2: LANDSCAPE 320x240 + RGB COLOR FIX    ");
    ESP_LOGI(TAG, "=================================================");

    // Backlight ON
    gpio_config_t bk_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BL,
    };
    gpio_config(&bk_conf);
    gpio_set_level(PIN_NUM_BL, 1);

    // DC and RST GPIO
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST),
    };
    gpio_config(&io_conf);

    // Hardware Reset
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    // SPI Bus init
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = H_RES * V_RES * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // SPI Device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 20 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &devcfg, &spi_dev));

    // 1. Sleep Out
    send_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));

    // 2. Pixel format: 16-bit RGB565
    send_cmd(0x3A);
    uint8_t colmod[] = { 0x55 };
    send_data(colmod, 1);

    // 3. Landscape 320x240 (Header on RIGHT) + RGB color mode (BGR bit = 0)
    // MADCTL bits: MY(0x80) | MX(0x00) | MV(0x20) = 0xA0 (Un-mirrored horizontal)
    send_cmd(0x36);
    uint8_t madctl[] = { 0xA0 };
    send_data(madctl, 1);

    // 4. Display ON
    send_cmd(0x29);
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "Launching test task: Full Red -> Green -> Blue -> 4 Quadrants");
    xTaskCreate(diag_task, "diag_task", 3072, NULL, 5, NULL);

    return ESP_OK;
}
