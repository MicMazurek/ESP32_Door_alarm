/*
 * wifi.c
 *
 *  Created on: 26 kwi 2026
 *      Author: mmazu
 */
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip_addr.h"
#include "nvs_flash.h"
#include "rom/ets_sys.h"
#include "string.h"
#define DEFAULT_SCAN_LIST_SIZE 10
//AP constants
// Define network credentials
#define WIFI_SSID      "Q60"
#define WIFI_PASS      "m4zurek123"

// Event Group bits to track connection status
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "WIFI_CLIENT";
static int s_retry_num = 0;
#define MAXIMUM_RETRY  5

// Event handler for Wi-Fi and IP events
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect(); // Start the actual Wi-Fi connection process
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ets_printf( "Retrying connection to the AP... %s\n",TAG);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ets_printf("Failed to connect to the AP %s\n",TAG);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ets_printf("Got IP address: %s\n",TAG);
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();

    // 1. Initialize the underlying TCP/IP network stack
    ESP_ERROR_CHECK(esp_netif_init());

    // 2. Create the system default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *netif_handle= esp_netif_create_default_wifi_sta();

    // 3. Allocate and set default configuration parameters
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // Mandatory initialization macro
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // Register event instances
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                            	            &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // 4. Configure credentials and strict settings
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold = { .authmode = WIFI_AUTH_WPA_WPA2_PSK }, // Default minimum security level
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    // 5. Start the Wi-Fi driver
    ESP_ERROR_CHECK(esp_wifi_start());

    ets_printf( "wifi_init_sta finished %s\n",TAG);

    // Blocking wait until connection is established or max retries hit
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ets_printf( "Connected to AP SSID: %s %s\n",TAG, WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
 		ets_printf( "Failed to connect to SSID: %s %s\n",TAG, WIFI_SSID);
    } else {
        ets_printf( "[%s] UNEXPECTED EVENT\n",TAG);
    }
}