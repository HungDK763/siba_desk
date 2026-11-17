#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "analog_joystick.h"

static const char *TAG = "JOYSTICK";

// Pin definitions on ESP32-S3 (Header J3)
#define JOY_PIN_VRX     GPIO_NUM_1   // ADC1_CH0
#define JOY_PIN_VRY     GPIO_NUM_2   // ADC1_CH1
#define JOY_PIN_SW      GPIO_NUM_42  // Digital Input with internal pullup

#define JOY_ADC_UNIT    ADC_UNIT_1
#define JOY_ADC_CH_X    ADC_CHANNEL_0
#define JOY_ADC_CH_Y    ADC_CHANNEL_1

#define JOY_ADC_MID     2048
#define JOY_DEADZONE    220          // Vùng chết chống trôi cần gạt

static adc_oneshot_unit_handle_t s_adc1_handle = NULL;
static bool s_initialized = false;

// Sub-pixel Accumulators cho thuật toán điều khiển chuột
static float s_mouse_accum_x = 0.0f;
static float s_mouse_accum_y = 0.0f;

// Cấu hình runtime mặc định
static analog_joystick_config_t s_config = {
    .swap_xy = JOY_SWAP_XY,
    .invert_x = JOY_INVERT_X,
    .invert_y = JOY_INVERT_Y,
    .mouse_deadzone = JOY_MOUSE_DEADZONE,
    .mouse_max_speed = JOY_MOUSE_MAX_SPEED,
    .mouse_return_thresh = JOY_MOUSE_RETURN_THRESH,
};

static int16_t s_prev_mouse_x = 0;
static int16_t s_prev_mouse_y = 0;

void analog_joystick_set_config(const analog_joystick_config_t *cfg) {
    if (cfg) {
        s_config = *cfg;
        ESP_LOGI(TAG, "⚙️ Cập nhật Joystick Config: SwapXY=%d, InvX=%d, InvY=%d, Deadzone=%d, MaxSpeed=%.1f, ReturnThresh=%d",
                 s_config.swap_xy, s_config.invert_x, s_config.invert_y,
                 s_config.mouse_deadzone, s_config.mouse_max_speed, s_config.mouse_return_thresh);
    }
}

void analog_joystick_get_config(analog_joystick_config_t *out_cfg) {
    if (out_cfg) {
        *out_cfg = s_config;
    }
}

esp_err_t analog_joystick_init(void) {
    if (s_initialized) {
        return ESP_OK;
    }

    // 1. Khởi tạo ADC1 Oneshot Unit
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = JOY_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &s_adc1_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Lỗi khởi tạo ADC1 unit: %s", esp_err_to_name(ret));
        return ret;
    }

    // 2. Cấu hình 2 kênh ADC1 cho trục X (GPIO 1) và trục Y (GPIO 2)
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, // 12-bit (0..4095)
        .atten = ADC_ATTEN_DB_12,         // Thang đo điện áp tối đa 3.3V
    };

    ret = adc_oneshot_config_channel(s_adc1_handle, JOY_ADC_CH_X, &chan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Lỗi cấu hình ADC1 CH_X (GPIO 1): %s", esp_err_to_name(ret));
        return ret;
    }

    ret = adc_oneshot_config_channel(s_adc1_handle, JOY_ADC_CH_Y, &chan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Lỗi cấu hình ADC1 CH_Y (GPIO 2): %s", esp_err_to_name(ret));
        return ret;
    }

    // 3. Cấu hình chân nút nhấn SW (GPIO 42) BẬT TRỞ KÉO LÊN NỘI (Internal Pull-Up)
    gpio_config_t sw_conf = {
        .pin_bit_mask = (1ULL << JOY_PIN_SW),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,     // ⚡ Kéo cao chân nút cần gạt
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&sw_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Lỗi cấu hình GPIO 42 (SW): %s", esp_err_to_name(ret));
        return ret;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "🕹️ Analog Joystick đã khởi tạo thành công:");
    ESP_LOGI(TAG, "   - Hoán đổi trục: %s (Trục X = VRY GPIO 2, Trục Y = VRX GPIO 1)", s_config.swap_xy ? "BẬT" : "TẮT");
    ESP_LOGI(TAG, "   - Đảo chiều:     Trục X=%s, Trục Y=%s", s_config.invert_x ? "BẬT" : "TẮT", s_config.invert_y ? "BẬT" : "TẮT");
    ESP_LOGI(TAG, "   - Nút SW:        GPIO %d (Internal Pull-Up ENABLED -> Chuột Trái)", JOY_PIN_SW);
    return ESP_OK;
}

static int16_t map_axis_with_deadzone(int raw) {
    if (raw < 0) raw = 0;
    if (raw > 4095) raw = 4095;

    int diff = raw - JOY_ADC_MID;
    if (abs(diff) <= JOY_DEADZONE) {
        return 0;
    }

    if (diff > 0) {
        int32_t val = (int32_t)(diff - JOY_DEADZONE) * 32767 / (JOY_ADC_MID - JOY_DEADZONE);
        if (val > 32767) val = 32767;
        return (int16_t)val;
    } else {
        int32_t val = (int32_t)(diff + JOY_DEADZONE) * 32768 / (JOY_ADC_MID - JOY_DEADZONE);
        if (val < -32768) val = -32768;
        return (int16_t)val;
    }
}

esp_err_t analog_joystick_read(joystick_state_t *out_state) {
    if (!out_state) return ESP_ERR_INVALID_ARG;
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    int raw_vrx = JOY_ADC_MID;
    int raw_vry = JOY_ADC_MID;

    adc_oneshot_read(s_adc1_handle, JOY_ADC_CH_X, &raw_vrx);
    adc_oneshot_read(s_adc1_handle, JOY_ADC_CH_Y, &raw_vry);

    // 🔄 HOÁN ĐỔI TRỤC THEO RUNTIME CONFIG:
    int raw_x, raw_y;
    if (s_config.swap_xy) {
        raw_x = raw_vry;
        raw_y = raw_vrx;
    } else {
        raw_x = raw_vrx;
        raw_y = raw_vry;
    }

    out_state->raw_x = (uint16_t)raw_x;
    out_state->raw_y = (uint16_t)raw_y;

    int16_t val_x = map_axis_with_deadzone(raw_x);
    int16_t val_y = map_axis_with_deadzone(raw_y);

    if (s_config.invert_x) {
        val_x = -val_x;
    }
    if (s_config.invert_y) {
        val_y = -val_y;
    }

    out_state->x = val_x;
    out_state->y = val_y;

    // Active LOW: 0 = Nhấn (đấu mass GND), 1 = Nhả (kéo cao 3.3V)
    out_state->button = (gpio_get_level(JOY_PIN_SW) == 0);

    return ESP_OK;
}

uint16_t analog_joystick_to_xbox_axis(int16_t norm_val) {
    int32_t v = (int32_t)norm_val + 32768;
    if (v < 0) v = 0;
    if (v > 65535) v = 65535;
    return (uint16_t)v;
}

/**
 * @brief Tính tốc độ di chuyển chuột cho từng trục:
 *        1. Độ lệch trong biên ổn định (deadzone) -> ĐỨNG YÊN (0.0f)
 *        2. Cần gạt có hướng đi về home (nhả cần) -> ĐỨNG YÊN (0.0f)
 *        3. Đang đẩy ra xa tâm hoặc giữ góc lệch ổn định -> Tốc độ tỉ lệ thuận với góc lệch
 */
static float calculate_axis_mouse_speed(int16_t curr_255, int16_t prev_255,
                                        int16_t deadzone, float max_speed, int16_t return_thresh) {
    int16_t abs_curr = abs(curr_255);
    int16_t abs_prev = abs(prev_255);

    // 1. Nếu nằm trong biên ổn định (Deadzone) quanh gốc 0 -> ĐỨNG YÊN
    if (abs_curr <= deadzone) {
        return 0.0f;
    }

    // 2. Kiểm tra nếu cùng phía (cùng dấu) nhưng đang có hướng co về tâm (nhả cần)
    bool same_sign = (curr_255 > 0 && prev_255 > 0) || (curr_255 < 0 && prev_255 < 0);
    if (same_sign && (abs_curr < abs_prev - return_thresh)) {
        // Cần gạt đang hướng về home -> ĐỨNG YÊN!
        return 0.0f;
    }

    // 3. Đang đẩy ra xa tâm hoặc giữ ổn định ở góc lệch ngoài deadzone
    float eff_disp = (float)(abs_curr - deadzone);
    float max_eff = (float)(255 - deadzone);
    if (max_eff < 1.0f) max_eff = 1.0f;
    float ratio = eff_disp / max_eff;
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio < 0.0f) ratio = 0.0f;

    // Tốc độ tỉ lệ với góc lệch (kết hợp lũy tiến nhẹ để rê chuột cực chuẩn ở vùng nhỏ)
    float speed = (ratio * ratio * 0.4f + ratio * 0.6f) * max_speed;

    return (curr_255 < 0) ? -speed : speed;
}

void analog_joystick_get_mouse_delta(const joystick_state_t *state, int8_t *out_dx, int8_t *out_dy) {
    if (!state || !out_dx || !out_dy) return;

    // 1. Tính tọa độ chuẩn hóa thang [-255, +255] từ raw_x và raw_y
    int diff_x = (int)state->raw_x - JOY_ADC_MID;
    int diff_y = (int)state->raw_y - JOY_ADC_MID;

    if (s_config.invert_x) diff_x = -diff_x;
    if (s_config.invert_y) diff_y = -diff_y;

    int16_t curr_x_255 = (int16_t)(diff_x * 255 / JOY_ADC_MID);
    int16_t curr_y_255 = (int16_t)(diff_y * 255 / JOY_ADC_MID);

    if (curr_x_255 > 255) curr_x_255 = 255;
    if (curr_x_255 < -255) curr_x_255 = -255;
    if (curr_y_255 > 255) curr_y_255 = 255;
    if (curr_y_255 < -255) curr_y_255 = -255;

    // 2. Tính tốc độ cho từng trục theo cấu hình và hướng chuyển động
    float speed_x = calculate_axis_mouse_speed(curr_x_255, s_prev_mouse_x,
                                               s_config.mouse_deadzone,
                                               s_config.mouse_max_speed,
                                               s_config.mouse_return_thresh);
    float speed_y = calculate_axis_mouse_speed(curr_y_255, s_prev_mouse_y,
                                               s_config.mouse_deadzone,
                                               s_config.mouse_max_speed,
                                               s_config.mouse_return_thresh);

    // Lưu lại vị trí để so sánh xu hướng di chuyển ở tick tiếp theo
    s_prev_mouse_x = curr_x_255;
    s_prev_mouse_y = curr_y_255;

    // 3. Tích lũy sub-pixel (reset khi trục đứng yên để tránh trôi ngầm)
    if (speed_x == 0.0f) {
        s_mouse_accum_x = 0.0f;
    } else {
        s_mouse_accum_x += speed_x;
    }

    if (speed_y == 0.0f) {
        s_mouse_accum_y = 0.0f;
    } else {
        s_mouse_accum_y += speed_y;
    }

    // 4. Lấy phần nguyên để gửi trong báo cáo USB/BLE HID
    int int_dx = (int)s_mouse_accum_x;
    int int_dy = (int)s_mouse_accum_y;

    s_mouse_accum_x -= (float)int_dx;
    s_mouse_accum_y -= (float)int_dy;

    // 5. Giới hạn dải int8_t (-127 .. 127)
    if (int_dx > 127) int_dx = 127;
    if (int_dx < -127) int_dx = -127;
    if (int_dy > 127) int_dy = 127;
    if (int_dy < -127) int_dy = -127;

    *out_dx = (int8_t)int_dx;
    *out_dy = (int8_t)int_dy;
}
