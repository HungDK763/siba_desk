#ifndef LAUNCHPAD_CORE_H
#define LAUNCHPAD_CORE_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LaunchPad core controller & GPIO input monitoring tasks
 */
esp_err_t launchpad_core_init(void);

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
