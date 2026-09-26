#include "WiFiTask.h"
#include "esp_sntp.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

static void InitWiFi(void);


void wifi_task(void *pvParam)
{
    while(1)
    {
        printf("Hello from WIFi Task\r\n");
        vTaskDelay(5000);
    }
}


static void InitWiFi(void)
{
    esp_netif_t *wifiConfigTemp;
    const char *hostName;

    // Init Others
//    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifiConfigTemp = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_netif_set_hostname(wifiConfigTemp, "smart_alarm"));
    ESP_ERROR_CHECK(esp_netif_get_hostname(wifiConfigTemp, &hostName));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    // ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    wifi_ps_type_t wifiPowerSaver;
    esp_wifi_get_ps(&wifiPowerSaver);
    printf("WiFi Power saving state is :%d\r\n", (uint8_t)wifiPowerSaver);
}

/*end of file*/