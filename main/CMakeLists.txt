idf_component_register(
    SRCS "app_main.cpp" "ui.cpp" "board_p4.cpp" "module_p4.c" "touch_gsl3680.cpp" "server_link.cpp" "fonts/eh3d_font_16.c" "fonts/eh3d_font_24.c"
    INCLUDE_DIRS "." ".."
    PRIV_REQUIRES driver app_update esp_app_format esp_timer spiffs nvs_flash esp_lcd esp_http_client fatfs sdmmc esp_driver_sdmmc esp_hw_support
)
