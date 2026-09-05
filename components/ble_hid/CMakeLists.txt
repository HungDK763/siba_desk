idf_component_register(SRCS "src/ble_hid.c"
                    INCLUDE_DIRS "include"
                    REQUIRES bt nvs_flash driver)
