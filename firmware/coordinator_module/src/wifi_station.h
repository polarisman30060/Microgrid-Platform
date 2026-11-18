#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#define APP_WIFI_SSID_MAX_LENGTH 32
#define APP_WIFI_PASSWORD_MAX_LENGTH 64
#define APP_WIFI_MAX_SCAN_RESULTS 12

typedef enum {
    APP_WIFI_STATE_STOPPED,
    APP_WIFI_STATE_DISCONNECTED,
    APP_WIFI_STATE_CONNECTING,
    APP_WIFI_STATE_CONNECTED,
    APP_WIFI_STATE_FAILED,
} app_wifi_state_t;

typedef struct {
    app_wifi_state_t state;
    char ssid[APP_WIFI_SSID_MAX_LENGTH + 1];
    char ipv4[16];
    esp_err_t last_error;
    uint8_t disconnect_reason;
    bool credentials_saved;
    bool web_server_ready;
} app_wifi_status_t;

typedef struct {
    char ssid[APP_WIFI_SSID_MAX_LENGTH + 1];
    int8_t rssi;
    bool secure;
} app_wifi_access_point_t;

esp_err_t app_wifi_station_start(void);
esp_err_t app_wifi_station_scan(void);
esp_err_t app_wifi_station_get_scan_results(
    app_wifi_access_point_t *access_points,
    size_t capacity,
    size_t *count);
esp_err_t app_wifi_station_connect(const char *ssid, const char *password);
esp_err_t app_wifi_station_get_status(app_wifi_status_t *status);
