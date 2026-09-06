idf_component_register(SRCS "src/launchpad_core.c"
                    INCLUDE_DIRS "include"
                    REQUIRES ble_hid driver freertos led_strip pc_sync)
