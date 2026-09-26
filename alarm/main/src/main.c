#include "main.h"
#include "WiFiTask.h"

BaseType_t retTask;


void app_main(void)
{
    backlight_init();
    display_init();
    backlight_set(80);

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

}


/*end of file*/