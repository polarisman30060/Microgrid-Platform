#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "web_server.h"

#define MAX_SCAN_RESULTS 10

static const char *TAG = "wifi";
static bool s_has_ssid;

static void log_scan_results(void)
{
    uint16_t found = 0;
    esp_err_t err = esp_wifi_scan_get_ap_num(&found);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not get access point count: %s", esp_err_to_name(err));
        return;
    }

    if (found == 0) {
        ESP_LOGI(TAG, "No access points found");
        return;
    }

    wifi_ap_record_t records[MAX_SCAN_RESULTS];
    uint16_t count = found < MAX_SCAN_RESULTS ? found : MAX_SCAN_RESULTS;
    err = esp_wifi_scan_get_ap_records(&count, records);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not read access point list: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Found %u access point(s); showing up to %u", found, count);
    for (uint16_t i = 0; i < count; ++i) {
        char ssid[sizeof(records[i].ssid) + 1];
        memcpy(ssid, records[i].ssid, sizeof(records[i].ssid));
        ssid[sizeof(records[i].ssid)] = '\0';
        ESP_LOGI(TAG, "  %s (%d dBm)", ssid, records[i].rssi);
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            if (s_has_ssid) {
                ESP_LOGI(TAG, "Connecting to configured access point");
                esp_err_t err = esp_wifi_connect();
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Wi-Fi connect failed: %s", esp_err_to_name(err));
                }
            } else {
                ESP_LOGI(TAG, "No SSID configured; scanning for nearby access points");
                esp_err_t err = esp_wifi_scan_start(NULL, false);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Wi-Fi scan failed: %s", esp_err_to_name(err));
                }
            }
        } else if (event_id == WIFI_EVENT_SCAN_DONE) {
            log_scan_results();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            const wifi_event_sta_disconnected_t *event = event_data;
            ESP_LOGW(TAG, "Disconnected from access point (reason %u)", event->reason);
            if (s_has_ssid) {
                esp_err_t err = esp_wifi_connect();
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Wi-Fi reconnect failed: %s", esp_err_to_name(err));
                }
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        esp_err_t err = app_web_server_start();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "HTTP server start failed: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Open http://" IPSTR "/index.html",
                     IP2STR(&event->ip_info.ip));
        }
    }
}

esp_err_t app_wifi_station_start(void)
{
    const char *ssid = CONFIG_APP_WIFI_SSID;
    const char *password = CONFIG_APP_WIFI_PASSWORD;
    const size_t ssid_length = strlen(ssid);
    const size_t password_length = strlen(password);
    wifi_config_t wifi_config = {0};
    if (ssid_length > sizeof(wifi_config.sta.ssid) ||
        password_length > sizeof(wifi_config.sta.password)) {
        return ESP_ERR_INVALID_SIZE;
    }
    s_has_ssid = ssid_length > 0;

    ESP_RETURN_ON_ERROR(nvs_flash_init(), TAG, "NVS init failed");
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Network interface init failed");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "Default event loop init failed");
    if (esp_netif_create_default_wifi_sta() == NULL) {
        return ESP_FAIL;
    }

    const wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init_config), TAG, "Wi-Fi driver init failed");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL),
        TAG, "Wi-Fi event registration failed");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL),
        TAG, "IP event registration failed");

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Wi-Fi station mode setup failed");
    if (s_has_ssid) {
        memcpy(wifi_config.sta.ssid, ssid, ssid_length);
        memcpy(wifi_config.sta.password, password, password_length);
        wifi_config.sta.threshold.authmode =
            password_length == 0 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
        wifi_config.sta.pmf_cfg.capable = true;
        wifi_config.sta.pmf_cfg.required = false;
        ESP_RETURN_ON_ERROR(
            esp_wifi_set_config(WIFI_IF_STA, &wifi_config),
            TAG, "Wi-Fi station credentials setup failed");
    }

    return esp_wifi_start();
}
