#include "WiFiTask.h"
#include "esp_sntp.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h" 
#include "TimeSync.h"

static void InitWiFi(void);
static void ConnectToWiFi(void);
static void WiFiDisconnect(void);
static void SyncRtcTime();

EventGroupHandle_t wifiEventGroup;
const int WIFI_CONNECTED_BIT = BIT0;
const int WIFI_FAIL_BIT      = BIT1;
WiFiStruct_t wifiData;
static const char *TAG = "WiFi_Task";
char deviceIpAdress[20] = "";
uint32_t tempTimeStarted = 0;
int s_retry_num = 0;
uint16_t apCount = 0;

/**
 * @brief Wi-Fi event handler function.
 *
 * This function handles various Wi-Fi and IP events. It logs messages and performs actions
 * based on the type of event received.
 *
 * @param arg User-defined argument (not used in this handler).
 * @param event_base The base ID of the event.
 * @param event_id The ID of the event.
 * @param event_data Additional data associated with the event.
 *
 * Events handled:
 * - WIFI_EVENT_STA_START: Logs that Wi-Fi has started and scanning is in progress.
 * - WIFI_EVENT_STA_DISCONNECTED: Attempts to reconnect to the access point (AP) up to 5 times.
 *   If reconnection fails after 5 attempts, sets the WIFI_FAIL_BIT in the event group.
 * - IP_EVENT_STA_GOT_IP: Logs the obtained IP address, stores it in the deviceIpAdress variable,
 *   and sets the WIFI_CONNECTED_BIT in the event group.
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event Handler Called EventBase: %s, Event ID: %d", (char *)event_base, (int)event_id);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "Wi-Fi started, scanning...");
    }
    else if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_DISCONNECTED) && (wifiData.isForceDisconnect == true))
    {
        wifiData.isForceDisconnect = false;
        ESP_LOGI(TAG, "Disconnected from WiFi SSID: %s", wifiData.connectedWiFiDetails.ssidOfNetwork);
        wifiData.isWiFiConnected = false;
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // Extract the reason code for the failed connection
        wifi_event_sta_disconnected_t* disconnected_event = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGW(TAG, "Connection Failed! Disconnect Reason code: %d", disconnected_event->reason);
        
        ESP_LOGI(TAG, "Setting Group Failed Bit");
        xEventGroupSetBits(wifiEventGroup, WIFI_FAIL_BIT);
        WiFiDisconnect();
    
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected! IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        sprintf(deviceIpAdress, IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
        tempTimeStarted = millis();
    
    }
}




void wifi_task(void *pvParam)
{
    InitWiFi();
    memcpy(wifiData.connectedWiFiDetails.ssidOfNetwork, DEFAULT_WIFI_SSID, strlen(DEFAULT_WIFI_SSID));
    memcpy(wifiData.connectedWiFiDetails.passOfNetwork, DEFAULT_WIFI_PASS, strlen(DEFAULT_WIFI_PASS));
    ConnectToWiFi();
    while(1)
    {
        // printf("Hello from WIFi Task\r\n");

        if (wifiData.isWiFiConnected && deviceData.isTimeSynced == false)
        {
            SyncRtcTime();
        }

        vTaskDelay(5000);
    }
}

static void SyncRtcTime()
{
    ForceCustomDNS();
    printf("Syncing time\r\n"); 
    time_t currentTime = 0;
    int retries = 3;
    
    printf("Syncing time\r\n");
    while(currentTime == 0 && retries > 0) 
    {
        currentTime = GetSntpTimeBlockingDNS();
        retries--;
    }


    if(currentTime > 0) 
    {
        deviceData.isTimeSynced = true;
        printf("Time acquired instantly in foreground!\n");
        printf("Time Taken: %ld\r\n", millis() - tempTimeStarted);

        time_t now;
        char strftime_buf[64];
        struct tm timeinfo;

        time(&now);
        printf("Time now is %lld\r\n", now);
        // Set timezone to UTC
        setenv("TZ", "UTC0", 1);
        tzset();

        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        printf( "The current date/time in UTC0 is: %s", strftime_buf);
    }

}


static void InitWiFi(void)
{
    esp_netif_t *wifiConfigTemp;
    const char *hostName;


    wifiEventGroup = xEventGroupCreate();
    if(wifiEventGroup == NULL)
    {
        ESP_LOGE(TAG , "Failed to create event group!");
    }

    // Init Others
   ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifiConfigTemp = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_netif_set_hostname(wifiConfigTemp, "smart_alarm"));
    ESP_ERROR_CHECK(esp_netif_get_hostname(wifiConfigTemp, &hostName));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    wifi_ps_type_t wifiPowerSaver;
    esp_wifi_get_ps(&wifiPowerSaver);
    printf("WiFi Power saving state is :%d\r\n", (uint8_t)wifiPowerSaver);
}


/**
 * @brief Attempts to connect to a Wi-Fi network using the credentials stored in wifiData.
 *
 * This function prepares the Wi-Fi configuration structure with the SSID and password
 * from the current wifiData, sets the Wi-Fi mode to station, and initiates the connection.
 * It waits for either a successful connection or a failure event, and updates the wifiData
 * status accordingly. On successful connection, it saves the Wi-Fi parameters to NVS.
 *
 * Steps performed:
 *  - Copies SSID and password from wifiData to temporary buffers.
 *  - Populates the wifi_config_t structure with the credentials.
 *  - Clears previous Wi-Fi event bits.
 *  - Sets Wi-Fi mode to station and applies the configuration.
 *  - Initiates the connection and waits for connection or failure event.
 *  - Updates connection status and saves parameters on success.
 *  - Handles connection failure and unexpected events.
 *
 * @note This function blocks until the connection attempt completes (success or failure).
 * @note Assumes wifiData, wifiEventGroup, and related resources are properly initialized.
 */
static void ConnectToWiFi(void) 
{
    wifi_config_t wifi_config;
    char tempWiFiSSID[32]; 
    char tempWiFiPass[63];
    EventBits_t bits;
    wifiData.isWiFiConnecting = true;
    // UpdateWiFiStatus();
    memset(&wifi_config, 0, sizeof(wifi_config_t));

    memcpy(tempWiFiSSID, wifiData.connectedWiFiDetails.ssidOfNetwork, sizeof(tempWiFiSSID));
    memcpy(tempWiFiPass, wifiData.connectedWiFiDetails.passOfNetwork, sizeof(tempWiFiPass));
    
    strncpy((char *)wifi_config.sta.ssid, tempWiFiSSID, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';  // Ensure null termination

    strncpy((char *)wifi_config.sta.password, tempWiFiPass, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';  // Ensure null termination

    // Clear all the previous events bits 
    xEventGroupClearBits(wifiEventGroup, WIFI_FAIL_BIT);
    xEventGroupClearBits(wifiEventGroup, WIFI_CONNECTED_BIT);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    s_retry_num = 0;
    do
    {
        if(s_retry_num > 0)
        {
            ESP_LOGI(TAG, "Retrying connection. Attempt: %d", s_retry_num);
        }
        ESP_LOGI(TAG, "Connecting to Wi-Fi: %s...", wifiData.connectedWiFiDetails.ssidOfNetwork);
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_ERROR_CHECK(esp_wifi_connect());

        bits = xEventGroupWaitBits(wifiEventGroup,
                WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                pdFALSE,
                pdFALSE,
                MAX_WIFI_CONNECTION_TIMEOUT);

        /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
        * happened. */
        if (bits & WIFI_CONNECTED_BIT) 
        {
            ESP_LOGI(TAG, "connected to ap SSID:%s ",
                    wifiData.connectedWiFiDetails.ssidOfNetwork);
            wifiData.isWiFiConnected = true;
            xEventGroupClearBits(wifiEventGroup, WIFI_CONNECTED_BIT);
            esp_wifi_set_ps(WIFI_PS_NONE);

        } 
        else if (bits & WIFI_FAIL_BIT) 
        {
            ESP_LOGI(TAG, "Failed to connect to SSID:%s ",
                    wifiData.connectedWiFiDetails.ssidOfNetwork);
            wifiData.isWiFiConnected = false;
            xEventGroupClearBits(wifiEventGroup, WIFI_FAIL_BIT);
        } 
        else 
        {
            xEventGroupClearBits(wifiEventGroup, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
            wifiData.isWiFiConnected = false;
            ESP_LOGE(TAG, "UNEXPECTED EVENT");
            // break;
        }
        s_retry_num++;
    }while(s_retry_num < 3 && wifiData.isWiFiConnected == false);
   
    printf("Turning isWiFiConnecting to false from line 562\r\n");
    wifiData.isWiFiConnecting = false;
}   


/**
 * @brief Disconnects from the current WiFi network and updates WiFi status flags.
 *
 * This function forcibly disconnects the device from the currently connected WiFi network
 * using the ESP-IDF API. It also updates the global wifiData structure to indicate that
 * a forced disconnect has occurred and that the WiFi connection is no longer active.
 *
 * @note This function assumes that the wifiData structure is accessible and properly initialized.
 */
static void WiFiDisconnect(void)
{
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    wifiData.isForceDisconnect = true;
    wifiData.isWiFiConnected = false;
}

unsigned long millis() 
{
  return (unsigned long)(esp_timer_get_time() / 1000ULL);
}
/*end of file*/