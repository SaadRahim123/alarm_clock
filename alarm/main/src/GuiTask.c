#include "GuiTask.h"
#include "ui/ui.h"
#include "time.h"



void gui_task(void *pvParam)
{
    time_t now, lastTimeUpdated = 0;
    time_t perSecondTick = 0;
    struct tm timeinfo;
    while(1)
    {
        if(deviceData.isTimeSynced)
        {
            // Get the current time
            time(&now);
            if ((now - lastTimeUpdated) > ONE_MINUTE_EPOCH_TIME)
            {
                lastTimeUpdated = now;

                // Get the time in time format
                localtime_r(&now, &timeinfo);
                // Update the ui
                printf("Updating time on UI\r\n");
                if (lvgl_port_lock(0)) 
                {
                    lv_label_set_text_fmt(ui_HoursLabel, "%02d", (timeinfo.tm_hour + deviceData.timezone_offset));
                    lv_label_set_text_fmt(ui_MinutesLabel, "%02d", timeinfo.tm_min);
                    lvgl_port_unlock();
                }
            }

            if ((now - perSecondTick) >= 1)
            {
                perSecondTick = now;

                if (lvgl_port_lock(0))
                {
                    if (lv_obj_has_flag(ui_Label3, LV_OBJ_FLAG_HIDDEN))
                    {
                        lv_obj_clear_flag(ui_Label3, LV_OBJ_FLAG_HIDDEN);
                    }
                    else
                    {
                        lv_obj_add_flag(ui_Label3, LV_OBJ_FLAG_HIDDEN);
                    }
                    lvgl_port_unlock();
                }
            }

        }
        vTaskDelay(100);
    }
}