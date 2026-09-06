file(GLOB_RECURSE UI_SRCS "ui/*.c")

idf_component_register(SRCS "src/display_ui.c" 
                            "src/touch_xpt2046.c" 
                            "src/earth_wallpaper.c" 
                            "src/font_sf_pro_rounded_60.c" 
                            "src/font_sf_pro_rounded_16.c" 
                            "src/mode_icons.c"
                            ${UI_SRCS}
                    INCLUDE_DIRS "include" "ui"
                    REQUIRES driver esp_timer lvgl esp_lcd espressif__esp_lvgl_port espressif__esp_lcd_ili9341 pc_sync ble_hid)

