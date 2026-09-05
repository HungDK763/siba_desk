idf_component_register(SRCS "esp_lanchpad_hid.c"
                    INCLUDE_DIRS "."
                    REQUIRES ble_hid launchpad_core nvs_flash)
