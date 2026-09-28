#ifndef _MAIN_H
#define _MAIN_H


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"   // esp_lcd_new_panel_st7789
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "ui/ui.h"                  // SquareLine export
#include "esp_log.h"
/* ---------- Board: ESP32-1732S019 (ST7789, SPI) ---------- */
#define LCD_HOST            SPI2_HOST
#define PIN_SCLK            12
#define PIN_MOSI            13
#define PIN_MISO            -1
#define PIN_CS              10
#define PIN_DC              11
#define PIN_RST             1
#define PIN_BL              14

#define LCD_H_RES           170
#define LCD_V_RES           320
#define LCD_X_GAP           35
#define LCD_Y_GAP           0

#define LCD_PIXEL_CLOCK_HZ  (24 * 1000 * 1000)
#define LCD_CMD_BITS        8
#define LCD_PARAM_BITS      8
#define LCD_BITS_PER_PIXEL  16

#define ONE_MINUTE_EPOCH_TIME   60

/*Threads parameters*/
#define WIFI_TASK_STACK_SIZE        4096
#define WIFI_TASK_PRIORITY          5
#define GUI_TASK_STACK_SIZE         4096
#define GUI_TASK_PRIORITY           5

typedef struct DeviceData_t
{
    bool isTimeSynced;
    int timezone_offset;
}DeviceData_t;


extern DeviceData_t deviceData;

/******FUNCTION PROTOTYPES *********/
void backlight_init(void);
void backlight_set(int pct);
lv_display_t *display_init(void);

unsigned long millis();
#endif  /*_MAIN_H*/