#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"
#include "touch_xpt2046.h"

static const char *TAG = "TOUCH_XPT";

// Default calibration parameters for 2.8" SPI TFT Landscape (Header on LEFT)
static touch_xpt2046_calib_t s_calib = {
    .min_raw_x = 200,
    .max_raw_x = 3850,
    .min_raw_y = 200,
    .max_raw_y = 3850,
    .swap_xy   = true,   // Landscape rotation exchanges X and Y
    .invert_x  = true,   // Inverted horizontally
    .invert_y  = true,   // Inverted vertically
};

static inline void touch_delay(void) {
    esp_rom_delay_us(2); // ~250 kHz clock, reliable and low noise
}

static uint8_t touch_spi_xfer(uint8_t tx) {
    uint8_t rx = 0;
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(TOUCH_PIN_MOSI, (tx >> i) & 1);
        gpio_set_level(TOUCH_PIN_CLK, 0);
        touch_delay();
        gpio_set_level(TOUCH_PIN_CLK, 1);
        touch_delay();
        if (gpio_get_level(TOUCH_PIN_MISO)) {
            rx |= (1 << i);
        }
    }
    gpio_set_level(TOUCH_PIN_CLK, 0);
    return rx;
}

static uint16_t touch_read_channel(uint8_t cmd) {
    gpio_set_level(TOUCH_PIN_CS, 0);
    touch_delay();
    touch_spi_xfer(cmd);
    uint8_t b1 = touch_spi_xfer(0x00);
    uint8_t b2 = touch_spi_xfer(0x00);
    gpio_set_level(TOUCH_PIN_CS, 1);
    touch_delay();
    return ((uint16_t)(b1 << 8) | b2) >> 3;
}

static uint16_t touch_read_channel_filtered(uint8_t cmd) {
    // 3 samples median filter
    uint16_t samples[3];
    for (int i = 0; i < 3; i++) {
        samples[i] = touch_read_channel(cmd);
    }
    if (samples[0] > samples[1]) { uint16_t t = samples[0]; samples[0] = samples[1]; samples[1] = t; }
    if (samples[1] > samples[2]) { uint16_t t = samples[1]; samples[1] = samples[2]; samples[2] = t; }
    if (samples[0] > samples[1]) { uint16_t t = samples[0]; samples[0] = samples[1]; samples[1] = t; }
    return samples[1];
}

esp_err_t touch_xpt2046_init(void) {
    ESP_LOGI(TAG, "Initializing XPT2046 Touch Controller...");
    ESP_LOGI(TAG, "Pins: IRQ=GPIO%d, DO(MISO)=GPIO%d, DIN(MOSI)=GPIO%d, CS=GPIO%d, CLK=GPIO%d",
             TOUCH_PIN_IRQ, TOUCH_PIN_MISO, TOUCH_PIN_MOSI, TOUCH_PIN_CS, TOUCH_PIN_CLK);

    // Configure Outputs (CS, CLK, MOSI)
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << TOUCH_PIN_CS) | (1ULL << TOUCH_PIN_CLK) | (1ULL << TOUCH_PIN_MOSI),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&out_conf));

    gpio_set_level(TOUCH_PIN_CS, 1);   // De-assert CS
    gpio_set_level(TOUCH_PIN_CLK, 0);  // Idle LOW (SPI Mode 0)
    gpio_set_level(TOUCH_PIN_MOSI, 0);

    // Configure Inputs (IRQ, MISO) with Pull-up
    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL << TOUCH_PIN_IRQ) | (1ULL << TOUCH_PIN_MISO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&in_conf));

    // Send dummy command to activate PENIRQ in power-down mode
    gpio_set_level(TOUCH_PIN_CS, 0);
    touch_spi_xfer(0x90); // Y-measurement with PD=00 (PENIRQ enabled)
    touch_spi_xfer(0x00);
    touch_spi_xfer(0x00);
    gpio_set_level(TOUCH_PIN_CS, 1);

    ESP_LOGI(TAG, "XPT2046 initialized. Initial IRQ level = %d (expected 1 when idle)",
             gpio_get_level(TOUCH_PIN_IRQ));

    return ESP_OK;
}

bool touch_xpt2046_read_raw(uint16_t *raw_x, uint16_t *raw_y) {
    if (gpio_get_level(TOUCH_PIN_IRQ) != 0) {
        return false; // Not touched (PENIRQ is active low)
    }

    uint16_t rx = touch_read_channel_filtered(0xD0); // X-channel (or Y depending on orientation)
    uint16_t ry = touch_read_channel_filtered(0x90); // Y-channel

    // Re-enable PENIRQ
    gpio_set_level(TOUCH_PIN_CS, 0);
    touch_spi_xfer(0x90);
    touch_spi_xfer(0x00);
    touch_spi_xfer(0x00);
    gpio_set_level(TOUCH_PIN_CS, 1);

    // Discard erratic boundary readings
    if (rx < 50 || rx > 4040 || ry < 50 || ry > 4040) {
        return false;
    }

    if (raw_x) *raw_x = rx;
    if (raw_y) *raw_y = ry;
    return true;
}

bool touch_xpt2046_read(uint16_t *x, uint16_t *y) {
    uint16_t rx = 0, ry = 0;
    if (!touch_xpt2046_read_raw(&rx, &ry)) {
        return false;
    }

    int val_x = s_calib.swap_xy ? ry : rx;
    int val_y = s_calib.swap_xy ? rx : ry;

    int min_x = s_calib.swap_xy ? s_calib.min_raw_y : s_calib.min_raw_x;
    int max_x = s_calib.swap_xy ? s_calib.max_raw_y : s_calib.max_raw_x;
    int min_y = s_calib.swap_xy ? s_calib.min_raw_x : s_calib.min_raw_y;
    int max_y = s_calib.swap_xy ? s_calib.max_raw_x : s_calib.max_raw_y;

    if (val_x < min_x) val_x = min_x;
    if (val_x > max_x) val_x = max_x;
    if (val_y < min_y) val_y = min_y;
    if (val_y > max_y) val_y = max_y;

    int scr_x = (int)((int64_t)(val_x - min_x) * 319 / (max_x - min_x));
    int scr_y = (int)((int64_t)(val_y - min_y) * 239 / (max_y - min_y));

    if (s_calib.invert_x) scr_x = 319 - scr_x;
    if (s_calib.invert_y) scr_y = 239 - scr_y;

    if (scr_x < 0) scr_x = 0;
    if (scr_x > 319) scr_x = 319;
    if (scr_y < 0) scr_y = 0;
    if (scr_y > 239) scr_y = 239;

    if (x) *x = (uint16_t)scr_x;
    if (y) *y = (uint16_t)scr_y;
    return true;
}

void touch_xpt2046_set_calib(const touch_xpt2046_calib_t *calib) {
    if (calib) {
        s_calib = *calib;
    }
}

void touch_xpt2046_get_calib(touch_xpt2046_calib_t *calib) {
    if (calib) {
        *calib = s_calib;
    }
}

// LVGL 8 Input Device Callback
static void touch_indev_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    uint16_t x, y;
    static uint16_t last_x = 160, last_y = 120;
    static int64_t last_log_time = 0;

    if (touch_xpt2046_read(&x, &y)) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
        last_x = x;
        last_y = y;

        int64_t now = esp_timer_get_time() / 1000;
        if (now - last_log_time > 200) {
            uint16_t rx, ry;
            touch_xpt2046_read_raw(&rx, &ry);
            ESP_LOGI("TOUCH", "👉 TOUCH: Raw=[%4d, %4d] -> Screen=[%3d, %3d]", rx, ry, x, y);
            last_log_time = now;
        }
    } else {
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

lv_indev_t* touch_xpt2046_register_lvgl(void) {
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_indev_read_cb;
    lv_indev_t *indev = lv_indev_drv_register(&indev_drv);
    ESP_LOGI(TAG, "LVGL Pointer Input Device registered successfully.");
    return indev;
}
