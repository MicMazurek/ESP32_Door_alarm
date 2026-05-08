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
#include "nvs_flash.h"
#include "rom/ets_sys.h"
#include "string.h"
#define DEFAULT_SCAN_LIST_SIZE 10
//AP constants
const char* SSID= "RPIM4azur";
const char* PASSWD = "rpigate33"; 
#define MAX_CLIENTS 4
#define CHANNEL 1
/*
void wifiAP_init(void)
{
    if(nvs_flash_init()!=ESP_OK){
		ets_printf("Critical failure\n");
	}
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    wifi_config_t conf = {
        .ap = {
            .ssid = "ESP32_MicMazurek",
            .password = "#EMIC$MazureK",
            .max_connection = MAX_CLIENTS,
            .channel = CHANNEL,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .ssid_hidden = 0,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &conf));
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_ERROR_CHECK(esp_wifi_start());
    printf("Wifi started\n");
}
static void print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        printf(TAG, "Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_OWE:
        printf(TAG, "Authmode \tWIFI_AUTH_OWE");
        break;
    case WIFI_AUTH_WEP:
        printf(TAG, "Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        printf(TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        printf(TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        printf(TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        printf(TAG, "Authmode \tWIFI_AUTH_WPA2_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_PSK:
        printf(TAG, "Authmode \tWIFI_AUTH_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        printf(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
        break;
    default:
        printf(TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }
}
*/
void wifiCli_init(void){
    nvs_flash_init();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_t *netif_handle = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(netif_handle, SSID);
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    wifi_config_t conf = {
        .sta = {
            .ssid = SSID,
            .password = PASSWD,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    uint8_t mac[6] = {0x70, 0xB3, 0xD5, 0x6A, 0x02, 0x01};
    esp_wifi_set_mac(WIFI_IF_STA, mac);
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &conf));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}
/*
void wifi_scan(void)
{
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    
    // Ustawia zera na wszystkie elementy ap_info
    memset(ap_info, 0, sizeof(ap_info));
    
    // Rozpoczęcie skanowania
    esp_wifi_scan_start(NULL, true);
    
        // Zapisuje w podanej przez nas zmiennej liczbę znalezionych Access Point'ów
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
     Zapisuje wszelkie informacje o pobliskich Access Point'ach
        w wcześniej utworzonej przez nas strukturze.
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    
    
    
    printf(TAG, "Liczba zeskanowanych AP's: %u", ap_count);
    for (uint8_t i=0; i<ap_count; i++){
        printf(TAG, "SSID \t\t%s", ap_info[i].ssid);
        printf(TAG, "RSSi \t\t%d", ap_info[i].rssi);
        print_auth_mode(ap_info[i].authmode);
        printf(TAG, "Channel \t\t%d\n", ap_info[i].primary);
    }
}*/