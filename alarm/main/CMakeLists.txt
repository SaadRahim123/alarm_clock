file(GLOB_RECURSE UI_SOURCES "ui/*.c")

idf_component_register(
    SRCS "src/display.c" "src/main.c" "src/WiFiTask.c" "src/TimeSync.c" "src/GuiTask.c" ${UI_SOURCES}
    INCLUDE_DIRS "." "ui" "inc/"
    REQUIRES esp_lcd esp_lvgl_port driver esp_timer esp_wifi esp_event nvs_flash
)