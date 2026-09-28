#ifndef _WIFI_TASK_H
#define _WIFI_TASK_H

#include "main.h"

#include "stdint.h"
#include "esp_wifi_types.h"

#define DEFAULT_WIFI_SSID        "HUAWEI-qF8F"
#define DEFAULT_WIFI_PASS       "vZT2EDvC"

#define ONE_MINUTE_TIMEOUT                  1000
#define RSSI_UPDATE_TIMEOUT                 20 * ONE_MINUTE_TIMEOUT


#define MAX_WIFI_SCAN_AP_COUNT              30
#define MAX_WIFI_APS_SAVED                  15
#define BUF_SIZE                            1024
#define WIFI_DATA_MAX_LIMIT                 256
#define MAX_AP_INFO_MESSAGES                MAX_WIFI_SCAN_AP_COUNT
#define SSID_MAX_LENGTH                     33
#define PASS_MAX_LENGTH                     64
#define CREDENTIALS_LENGTH_BOUND            24
#define MAX_WIFI_CONNECTION_TIMEOUT         10000

// WiFi Status Bits
#define WIFI_ENABLED_BIT                  0
#define WIFI_INITIALIZED                  1
#define WIFI_CONNECTED_STATUS_BIT         2
#define WIFI_ERROR_BIT                    3
#define WIFI_CONNECTING_BIT               4
#define WIFI_SCAN_BIT                     5

typedef enum WiFiTaskState
{
    WIFI_DISCONNECTED       = 0,
    WIFI_INIT_STATE         = 1,
    WIFI_SCAN_STATE         = 2,
    WIFI_CONNECTION_STATE   = 3,
    WIFI_RUNNING_STATE      = 4,
    WIFI_ERROR_STATE        = 5,
    WIFI_TASK_WAITING_STATE = 6,
}WiFiTaskState;

typedef struct WiFiNetworkDetails_t
{
    char ssidOfNetwork[SSID_MAX_LENGTH];
    char passOfNetwork[PASS_MAX_LENGTH];
}WiFiNetworkDetails_t;


typedef enum WiFiTaskCommands
{
    WIFI_ENABLE_COMMAND             = 0,
    WIFI_SCAN_COMMAND               = 1,
    WIFI_CONNECTION_COMMAND         = 2,
    WIFI_STATUS_COMMAND             = 3,
}WiFiTaskCommands;

typedef struct WiFiMessage_t
{
    uint16_t command;
    uint8_t data[WIFI_DATA_MAX_LIMIT];
    uint8_t messageLength;
}WiFiMessage_t;



typedef struct WiFiStruct_t
{
    bool isEnabled;
    bool isWiFiInitialized;
    bool isWiFiConnected;
    bool isWiFiConnecting;
    bool isWiFiScanning;
    bool isWiFiCredentialsReceived;
    bool isWiFiInError;
    bool isWiFiParametersRetrievedFromNVS;
    bool isForceDisconnect;
    WiFiTaskState wifiStateMachine;
    wifi_ap_record_t apInfo[MAX_AP_INFO_MESSAGES];
    WiFiNetworkDetails_t connectedWiFiDetails;
    WiFiNetworkDetails_t lastConnectedWiFiDetails;
    uint16_t wifiApPresent;                     //!< Number of APs present
    uint8_t wifiStatus;                         //!< Status of WiFi
    uint8_t errorCode;                          //!< Holds the Error Code
    uint8_t wifiApConnectedIndex;               //!< This parameter holds the connected wifi place in the list of wifi APs
    WiFiNetworkDetails_t savedWifiAccessPoints[MAX_WIFI_APS_SAVED];     //!< Holds the saved WiFi access points
}WiFiStruct_t;




void wifi_task(void *pvParam);

#endif  /*_WIFI_TASK_H*/