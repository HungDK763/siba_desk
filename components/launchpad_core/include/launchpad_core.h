#ifndef LAUNCHPAD_CORE_H
#define LAUNCHPAD_CORE_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LP_MODE_GAMEPAD = 0,
    LP_MODE_KB_MOUSE = 1,
    LP_MODE_MAX
} launchpad_mode_t;

/**
 * @brief Initialize LaunchPad core controller & GPIO input monitoring tasks
 */
esp_err_t launchpad_core_init(void);

/**
 * @brief Set current input mode (Gamepad or Keyboard+Mouse)
 */
void launchpad_core_set_mode(launchpad_mode_t mode);

/**
 * @brief Get current input mode
 */
launchpad_mode_t launchpad_core_get_mode(void);

/**
 * @brief Trigger a pad event on the LaunchPad
 * 
 * @param pad_index Index of the pad (0 to 15)
 * @param pressed true if pressed, false if released
 */
void launchpad_core_trigger_pad(uint8_t pad_index, bool pressed);

#ifdef __cplusplus
}
#endif

#endif // LAUNCHPAD_CORE_H
