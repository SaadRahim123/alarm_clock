#include "main.h"
#include "WiFiTask.h"
#include "GuiTask.h"

BaseType_t retTask;
DeviceData_t deviceData;

static void config_device_param();

void app_main(void)
{
    backlight_init();
    display_init();
    backlight_set(80);
    config_device_param();
    if (lvgl_port_lock(0)) {
        ui_init();          // SquareLine screens
        lvgl_port_unlock();
    }


    retTask =  xTaskCreate( wifi_task,
                         "WiFi Task",
                         WIFI_TASK_STACK_SIZE,
                         NULL,
                         WIFI_TASK_PRIORITY,
                         NULL
                       );


    retTask =  xTaskCreate( gui_task,
                         "GUI Task",
                         GUI_TASK_STACK_SIZE,
                         NULL,
                         GUI_TASK_PRIORITY,
                         NULL
                       );


}

static void config_device_param()
{
    deviceData.timezone_offset = 5;
}
/*end of file*/