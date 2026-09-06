#ifndef RS485_BUS_H
#define RS485_BUS_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Khởi tạo cổng truyền thông RS485 trên UART1 phần cứng
 * @param baud_rate Tốc độ baud (ví dụ 115200 hoặc 9600)
 */
esp_err_t rs485_bus_init(uint32_t baud_rate);

/**
 * @brief Gửi gói tin qua đường truyền RS485
 */
esp_err_t rs485_bus_send(const uint8_t *data, size_t len);

/**
 * @brief Nhận dữ liệu từ đường truyền RS485 (non-blocking hoặc có timeout)
 */
int rs485_bus_receive(uint8_t *buffer, size_t max_len, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // RS485_BUS_H
