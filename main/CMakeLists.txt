idf_component_register(
    SRCS "app_main.cpp" "ui.cpp" "board_p4.cpp" "server_link.cpp" "fonts/eh3d_font_16.c" "fonts/eh3d_font_24.c"
    INCLUDE_DIRS "." ".."
    PRIV_REQUIRES driver esp_timer spiffs nvs_flash esp_lcd esp_http_client fatfs sdmmc
)
