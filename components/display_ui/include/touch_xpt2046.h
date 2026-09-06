#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Touch Pinout according to hardware documentation (Header J1)
#define TOUCH_PIN_IRQ     GPIO_NUM_16   // J1-9  (T_IRQ)
#define TOUCH_PIN_MISO    GPIO_NUM_17   // J1-10 (T_DO)
#define TOUCH_PIN_MOSI    GPIO_NUM_18   // J1-11 (T_DIN)
#define TOUCH_PIN_CS      GPIO_NUM_8    // J1-12 (T_CS)
#define TOUCH_PIN_CLK     GPIO_NUM_3    // J1-13 (T_CLK)

/**
 * @brief Calibration & Orientation configuration
 */
typedef struct {
    int min_raw_x;
    int max_raw_x;
    int min_raw_y;
    int max_raw_y;
    bool swap_xy;
    bool invert_x;
    bool invert_y;
} touch_xpt2046_calib_t;

/**
 * @brief Initialize XPT2046 GPIOs and hardware
 */
esp_err_t touch_xpt2046_init(void);

/**
 * @brief Read debounced touch coordinates in screen space (0..319, 0..239)
 * @param[out] x Screen X coordinate
 * @param[out] y Screen Y coordinate
 * @return true if currently touched, false otherwise
 */
bool touch_xpt2046_read(uint16_t *x, uint16_t *y);

/**
 * @brief Read raw ADC coordinates directly (for diagnostics/calibration)
 * @param[out] raw_x Raw ADC X (0..4095)
 * @param[out] raw_y Raw ADC Y (0..4095)
 * @return true if touched, false otherwise
 */
bool touch_xpt2046_read_raw(uint16_t *raw_x, uint16_t *raw_y);

/**
 * @brief Update calibration parameters
 */
void touch_xpt2046_set_calib(const touch_xpt2046_calib_t *calib);

/**
 * @brief Get current calibration parameters
 */
void touch_xpt2046_get_calib(touch_xpt2046_calib_t *calib);

/**
 * @brief Register XPT2046 as an LVGL pointer input device
 * @return Pointer to registered lv_indev_t
 */
lv_indev_t* touch_xpt2046_register_lvgl(void);

#ifdef __cplusplus
}
#endif
