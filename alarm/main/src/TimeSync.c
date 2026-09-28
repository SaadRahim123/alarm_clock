/**
 * @file TimeSync.cpp
 * @author Muhammad Saad Rahim
 * @brief Handling All Audio Handling
 * @date 2026-04-10
 *
 * @copyright COBRA-FIRING SYSTEMS Copyright (c) 2025
 *
 */
#include "TimeSync.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include <time.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_wifi.h"

////////////////// Global Variables ////////////////////
const char *TAG_SNTP = "SNTP";
///////////////////////////////////////////////////////

////////////////// Extern Variables ////////////////////
extern uint32_t tempTimeStarted;
///////////////////////////////////////////////////////


////////////////// Static Functions ////////////////////
static void TimeSyncNotificationCb(struct timeval *tv);

///////////////////////////////////////////////////////

/**
 * @brief Callback for time syncing in case of daemon task sntp
 * 
 * This function is called by the daemon task as a callback when time is retrieved from
 * sntp server.
 * 
 * @return void
 * 
 */
static void TimeSyncNotificationCb(struct timeval *tv)
{
    // This callback runs when the time is successfully synchronized
    deviceData.isTimeSynced = true;
    printf("SNTP Synced! Real global time updated.\n");
    printf("Time taken in syncing is %ld ms\r\n", millis() - tempTimeStarted);

    // 1. Get the seconds from the provided timeval struct
    time_t now = tv->tv_sec;
    struct tm timeinfo;

    // wait for time to be set
    time(&now);
    localtime_r(&now, &timeinfo);

    char strftime_buf[64];

    // 3. Format the time into a string (e.g., "Fri Apr 10 12:29:25 2026")
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    printf( "SNTP Synced! Current real global time is: %s", strftime_buf);

}

/**
 * @brief Retrieves the current global epoch time in seconds since Unix epoch.
 * 
 * This function returns the current system time as a 64-bit unsigned integer
 * representing the number of seconds since January 1, 1970 (Unix epoch).
 * 
 * @return uint64_t The current epoch time in seconds if time synchronization is active,
 *                   or 0 if the system time has not been synchronized.
 * 
 * @see time() for the underlying system time retrieval function.
 */
uint64_t GetGlobalEpochTime(void)
{
    time_t now;
    if (deviceData.isTimeSynced == false)
    {
        return 0;
    }
    time(&now);
    return (uint64_t) now;
}

/**
 * @brief The function initializes and sync the time from ntp using daemon task approach
 * 
 * This function configures the daemon task that is responsible for getting the time from 
 * SNTP global servers. The function also sets the custom callback for the daemon task to configure it
 * and calls it when the sntp time is retrieved.
 * 
 * @return void
 * 
 */
void InitializeandSyncSntp(void)
{
    printf("Initializing SNTP...\n");

    // Configure the SNTP service using the default pool

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(
        2, // Number of servers in the list
        ESP_SNTP_SERVER_LIST(
            "pool.ntp.org",       // Standard global pool
            "time.google.com"    // Highly responsive Google time server
        )
    );

    // Attach your callback
    config.sync_cb = TimeSyncNotificationCb; 

    // This kicks off the background service. It will wait for Wi-Fi on its own!
    esp_netif_sntp_init(&config);

}


/**
 * @brief Retrieves the current time from an NTP server in a blocking manner.
 * 
 * This function establishes a UDP connection to a public NTP server (Cloudflare),
 * sends an NTP request, and waits for a response with a 3-second timeout.
 * Upon successful reception of a valid NTP response, it extracts the timestamp,
 * converts it to Unix time, and updates the system clock.
 * 
 * @return time_t The current Unix timestamp (seconds since epoch) if successful,
 *                or 0 if the operation fails (socket creation failed, send failed,
 *                receive timeout, or invalid response packet).
 */
time_t GetSntpTimeBlocking(void) 
{
    // 1. Create a UDP Socket
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        return 0; // Socket creation failed
    }

    // 2. Set a strict 3-second receive timeout so your task doesn't hang
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // 3. Setup the NTP Server address (Using Cloudflare's IP to avoid DNS delays)
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(123); // Standard NTP Port
    inet_pton(AF_INET, "162.159.200.1", &dest_addr.sin_addr);

    // 4. Prepare the standard 48-byte NTP Client payload
    uint8_t ntp_msg[48] = {0};
    ntp_msg[0] = 0x1B; // LI = 0, VN = 3, Mode = 3 (Client)

    // 5. Send the UDP request (Executes instantly)
    if (sendto(sock, ntp_msg, sizeof(ntp_msg), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        close(sock);
        return 0; // Send failed
    }

    // 6. Block the task and wait for the response
    struct sockaddr_in source_addr;
    socklen_t addr_len = sizeof(source_addr);
    int len = recvfrom(sock, ntp_msg, sizeof(ntp_msg), 0, (struct sockaddr *)&source_addr, &addr_len);
    
    close(sock);

    // 7. If we got the 48-byte response, parse the time
    if (len == 48) {
        // Extract the 32-bit seconds field from the payload (bytes 40-43)
        uint32_t ntp_time = (ntp_msg[40] << 24) | (ntp_msg[41] << 16) | (ntp_msg[42] << 8) | ntp_msg[43];
        
        // Convert NTP time to standard Unix Time
        time_t unix_time = ntp_time - NTP_TIMESTAMP_DELTA;
        
        // Update the ESP32's internal system clock right now
        struct timeval tv = { .tv_sec = unix_time, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        
        return unix_time;
    }

    // Packet dropped or timeout reached
    return 0; 
}

/**
 * @brief Retrieves the current time from an NTP server in a blocking manner with dns resolution.
 * 
 * This function establishes a UDP connection to a public NTP server (Cloudflare),
 * sends an NTP request, and waits for a response with a 3-second timeout.
 * Upon successful reception of a valid NTP response, it extracts the timestamp,
 * converts it to Unix time, and updates the system clock.
 * 
 * @return time_t The current Unix timestamp (seconds since epoch) if successful,
 *                or 0 if the operation fails (socket creation failed, send failed,
 *                receive timeout, or invalid response packet).
 */
time_t GetSntpTimeBlockingDNS(void) 
{
    // 1. Create a UDP Socket
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        return 0; // Socket creation failed
    }

    // 2. Set a strict 3-second receive timeout
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // 3. Resolve the domain name to an IP address (DNS Lookup)
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // We want an IPv4 address
    hints.ai_socktype = SOCK_DGRAM; // We want a UDP socket

    // This will block momentarily while it asks the DNS server for the IP
    if (getaddrinfo("pool.ntp.org", "123", &hints, &res) != 0) {
        
        close(sock);
        return 0; // DNS resolution failed (timeout or no internet)
    }

    // 4. Prepare the standard 48-byte NTP Client payload
    uint8_t ntp_msg[48] = {0};
    ntp_msg[0] = 0x1B; // LI = 0, VN = 3, Mode = 3 (Client)

    // 5. Send the UDP request using the resolved address from 'res'
    if (sendto(sock, ntp_msg, sizeof(ntp_msg), 0, res->ai_addr, res->ai_addrlen) < 0) {
        freeaddrinfo(res); // Free DNS memory
        close(sock);
        return 0; // Send failed
    }

    // Free the DNS result list memory as we don't need it anymore
    freeaddrinfo(res); 

    // 6. Block the task and wait for the response
    struct sockaddr_in source_addr;
    socklen_t addr_len = sizeof(source_addr);
    int len = recvfrom(sock, ntp_msg, sizeof(ntp_msg), 0, (struct sockaddr *)&source_addr, &addr_len);
    
    close(sock);

    // 7. If we got the 48-byte response, parse the time
    if (len == 48) {
        // Extract the 32-bit seconds field from the payload (bytes 40-43)
        uint32_t ntp_time = (ntp_msg[40] << 24) | (ntp_msg[41] << 16) | (ntp_msg[42] << 8) | ntp_msg[43];
        
        // Convert NTP time to standard Unix Time
        time_t unix_time = ntp_time - NTP_TIMESTAMP_DELTA;
        
        // Update the ESP32's internal system clock right now
        struct timeval tv = { .tv_sec = unix_time, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        return unix_time;
    }

    // Packet dropped or timeout reached
    return 0; 
}


/**
 * @brief Forces custom DNS servers for WiFi station interface
 * 
 * Configures the WiFi STA network interface to use Google's primary DNS (8.8.8.8)
 * and Cloudflare's secondary DNS (1.1.1.1) servers. This overrides any DNS settings
 * obtained from DHCP or manual configuration.
 * 
 * @note This function assumes the WiFi STA interface is already initialized.
 *       If the interface handle is invalid, the function may fail silently.
 * 
 * @return void
 * 
 */
void ForceCustomDNS() 
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_dns_info_t dns_info;

    // Set primary DNS to Google (8.8.8.8)
    IP4_ADDR(&dns_info.ip.u_addr.ip4, 8, 8, 8, 8);
    dns_info.ip.type = IPADDR_TYPE_V4;
    esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);

    // Set secondary DNS to Cloudflare (1.1.1.1)
    IP4_ADDR(&dns_info.ip.u_addr.ip4, 1, 1, 1, 1);
    dns_info.ip.type = IPADDR_TYPE_V4;
    esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
}

/*end of file*/