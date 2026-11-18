#include <stdio.h>
#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "web_server.h"
#include "wifi_station.h"

#define WIFI_NVS_NAMESPACE "wifi_cfg"
#define WIFI_NVS_SSID_KEY "ssid"
#define WIFI_NVS_PASSWORD_KEY "password"
#define WIFI_RETRY_LIMIT 3

static const char *TAG = "wifi";
static portMUX_TYPE s_wifi_lock = portMUX_INITIALIZER_UNLOCKED;
static app_wifi_status_t s_status = {.state = APP_WIFI_STATE_STOPPED};
static app_wifi_access_point_t s_scan_results[APP_WIFI_MAX_SCAN_RESULTS];
static size_t s_scan_result_count;
static esp_err_t s_scan_error;
static bool s_scan_in_progress;
static bool s_scan_complete;
static bool s_wifi_started;
static bool s_has_credentials;
static uint8_t s_retry_count;
static char s_active_ssid[APP_WIFI_SSID_MAX_LENGTH + 1];
static char s_active_password[APP_WIFI_PASSWORD_MAX_LENGTH + 1];

static esp_err_t validate_credentials(const char *ssid, const char *password)
{
    if (ssid == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t ssid_length = strnlen(ssid, APP_WIFI_SSID_MAX_LENGTH + 1);
    size_t password_length = strnlen(password, APP_WIFI_PASSWORD_MAX_LENGTH + 1);
    if (ssid_length == 0 || ssid_length > APP_WIFI_SSID_MAX_LENGTH ||
        password_length > APP_WIFI_PASSWORD_MAX_LENGTH) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (password_length > 0 && password_length < 8) {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

static esp_err_t load_saved_credentials(char *ssid, char *password)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }

    size_t ssid_size = APP_WIFI_SSID_MAX_LENGTH + 1;
    size_t password_size = APP_WIFI_PASSWORD_MAX_LENGTH + 1;
    err = nvs_get_str(handle, WIFI_NVS_SSID_KEY, ssid, &ssid_size);
    if (err == ESP_OK) {
        err = nvs_get_str(handle, WIFI_NVS_PASSWORD_KEY, password, &password_size);
    }
    nvs_close(handle);
    if (err != ESP_OK) {
        return err;
    }

    return validate_credentials(ssid, password);
}

static esp_err_t save_active_credentials(void)
{
    char ssid[APP_WIFI_SSID_MAX_LENGTH + 1];
    char password[APP_WIFI_PASSWORD_MAX_LENGTH + 1];
    portENTER_CRITICAL(&s_wifi_lock);
    memcpy(ssid, s_active_ssid, sizeof(ssid));
    memcpy(password, s_active_password, sizeof(password));
    portEXIT_CRITICAL(&s_wifi_lock);

    nvs_handle_t handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(handle, WIFI_NVS_SSID_KEY, ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(handle, WIFI_NVS_PASSWORD_KEY, password);
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

static void set_connection_failed(esp_err_t error)
{
    portENTER_CRITICAL(&s_wifi_lock);
    s_status.state = APP_WIFI_STATE_FAILED;
    s_status.last_error = error;
    s_status.ipv4[0] = '\0';
    portEXIT_CRITICAL(&s_wifi_lock);
}

static void handle_scan_done(void)
{
    uint16_t found = 0;
    esp_err_t err = esp_wifi_scan_get_ap_num(&found);
    wifi_ap_record_t records[APP_WIFI_MAX_SCAN_RESULTS] = {0};
    uint16_t count = found < APP_WIFI_MAX_SCAN_RESULTS ? found : APP_WIFI_MAX_SCAN_RESULTS;

    if (err == ESP_OK && count > 0) {
        err = esp_wifi_scan_get_ap_records(&count, records);
    }

    portENTER_CRITICAL(&s_wifi_lock);
    s_scan_result_count = 0;
    s_scan_error = err;
    if (err == ESP_OK) {
        for (uint16_t i = 0; i < count; ++i) {
            size_t ssid_length = strnlen(
                (const char *)records[i].ssid, sizeof(records[i].ssid));
            if (ssid_length == 0) {
                continue;
            }

            app_wifi_access_point_t *result = &s_scan_results[s_scan_result_count++];
            memcpy(result->ssid, records[i].ssid, ssid_length);
            result->ssid[ssid_length] = '\0';
            result->rssi = records[i].rssi;
            result->secure = records[i].authmode != WIFI_AUTH_OPEN;
        }
    }
    s_scan_in_progress = false;
    s_scan_complete = true;
    portEXIT_CRITICAL(&s_wifi_lock);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi scan results failed: %s", esp_err_to_name(err));
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            portENTER_CRITICAL(&s_wifi_lock);
            bool has_credentials = s_has_credentials;
            if (has_credentials) {
                s_status.state = APP_WIFI_STATE_CONNECTING;
            } else {
                s_status.state = APP_WIFI_STATE_DISCONNECTED;
            }
            portEXIT_CRITICAL(&s_wifi_lock);

            if (has_credentials) {
                esp_err_t err = esp_wifi_connect();
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Initial Wi-Fi connection request failed: %s", esp_err_to_name(err));
                    set_connection_failed(err);
                }
            }
        } else if (event_id == WIFI_EVENT_SCAN_DONE) {
            handle_scan_done();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            const wifi_event_sta_disconnected_t *event = event_data;
            bool retry = false;

            portENTER_CRITICAL(&s_wifi_lock);
            s_status.ipv4[0] = '\0';
            s_status.disconnect_reason = event->reason;
            s_status.web_server_ready = false;
            if (s_has_credentials && s_retry_count < WIFI_RETRY_LIMIT) {
                ++s_retry_count;
                s_status.state = APP_WIFI_STATE_CONNECTING;
                retry = true;
            } else {
                s_status.state = APP_WIFI_STATE_FAILED;
                s_status.last_error = ESP_FAIL;
            }
            portEXIT_CRITICAL(&s_wifi_lock);

            ESP_LOGW(TAG, "Wi-Fi disconnected (reason %u)", event->reason);
            if (retry) {
                esp_err_t err = esp_wifi_connect();
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Wi-Fi reconnect request failed: %s", esp_err_to_name(err));
                    set_connection_failed(err);
                }
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = event_data;
        char ip_address[sizeof(s_status.ipv4)];
        snprintf(ip_address, sizeof(ip_address), IPSTR, IP2STR(&event->ip_info.ip));

        portENTER_CRITICAL(&s_wifi_lock);
        s_status.state = APP_WIFI_STATE_CONNECTED;
        s_status.last_error = ESP_OK;
        s_status.disconnect_reason = 0;
        s_status.credentials_saved = false;
        s_retry_count = 0;
        strcpy(s_status.ipv4, ip_address);
        portEXIT_CRITICAL(&s_wifi_lock);

        ESP_LOGI(TAG, "Wi-Fi connected; open http://%s/", ip_address);
        esp_err_t err = save_active_credentials();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Could not save Wi-Fi credentials: %s", esp_err_to_name(err));
            portENTER_CRITICAL(&s_wifi_lock);
            s_status.last_error = err;
            portEXIT_CRITICAL(&s_wifi_lock);
        } else {
            portENTER_CRITICAL(&s_wifi_lock);
            s_status.credentials_saved = true;
            portEXIT_CRITICAL(&s_wifi_lock);
        }

        err = app_web_server_start();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "HTTP server start failed: %s", esp_err_to_name(err));
        }
        portENTER_CRITICAL(&s_wifi_lock);
        s_status.web_server_ready = err == ESP_OK;
        portEXIT_CRITICAL(&s_wifi_lock);
    }
}

esp_err_t app_wifi_station_start(void)
{
    char ssid[APP_WIFI_SSID_MAX_LENGTH + 1] = {0};
    char password[APP_WIFI_PASSWORD_MAX_LENGTH + 1] = {0};
    esp_err_t credential_err;
    wifi_config_t wifi_config = {0};

    ESP_RETURN_ON_ERROR(nvs_flash_init(), TAG, "NVS init failed");
    credential_err = load_saved_credentials(ssid, password);
    if (credential_err == ESP_OK) {
        memcpy(s_active_ssid, ssid, sizeof(s_active_ssid));
        memcpy(s_active_password, password, sizeof(s_active_password));
        s_has_credentials = true;
        memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
        memcpy(wifi_config.sta.password, password, strlen(password));
        wifi_config.sta.threshold.authmode =
            password[0] == '\0' ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
        wifi_config.sta.pmf_cfg.capable = true;
        wifi_config.sta.pmf_cfg.required = false;
    } else if (credential_err == ESP_ERR_NVS_NOT_FOUND) {
        s_has_credentials = false;
        ESP_LOGI(TAG, "No saved Wi-Fi network; configure one from the LCD");
    } else {
        s_has_credentials = false;
        ESP_LOGE(TAG, "Stored Wi-Fi credentials are invalid: %s", esp_err_to_name(credential_err));
        portENTER_CRITICAL(&s_wifi_lock);
        s_status.last_error = credential_err;
        portEXIT_CRITICAL(&s_wifi_lock);
    }

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
    if (s_has_credentials) {
        ESP_RETURN_ON_ERROR(
            esp_wifi_set_config(WIFI_IF_STA, &wifi_config),
            TAG, "Wi-Fi station credentials setup failed");
    }

    esp_err_t err = esp_wifi_start();
    if (err == ESP_OK) {
        portENTER_CRITICAL(&s_wifi_lock);
        s_wifi_started = true;
        s_status.state = s_has_credentials ? APP_WIFI_STATE_CONNECTING : APP_WIFI_STATE_DISCONNECTED;
        memcpy(s_status.ssid, ssid, sizeof(s_status.ssid));
        portEXIT_CRITICAL(&s_wifi_lock);
    }
    return err;
}

esp_err_t app_wifi_station_scan(void)
{
    portENTER_CRITICAL(&s_wifi_lock);
    if (!s_wifi_started) {
        portEXIT_CRITICAL(&s_wifi_lock);
        return ESP_ERR_INVALID_STATE;
    }
    if (s_scan_in_progress) {
        portEXIT_CRITICAL(&s_wifi_lock);
        return ESP_ERR_NOT_FINISHED;
    }
    s_scan_in_progress = true;
    s_scan_complete = false;
    s_scan_result_count = 0;
    portEXIT_CRITICAL(&s_wifi_lock);

    esp_err_t err = esp_wifi_scan_start(NULL, false);
    if (err != ESP_OK) {
        portENTER_CRITICAL(&s_wifi_lock);
        s_scan_in_progress = false;
        s_scan_complete = true;
        s_scan_error = err;
        portEXIT_CRITICAL(&s_wifi_lock);
    }
    return err;
}

esp_err_t app_wifi_station_get_scan_results(
    app_wifi_access_point_t *access_points,
    size_t capacity,
    size_t *count)
{
    if (access_points == NULL || count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&s_wifi_lock);
    if (!s_scan_complete) {
        portEXIT_CRITICAL(&s_wifi_lock);
        return ESP_ERR_NOT_FINISHED;
    }
    if (s_scan_error != ESP_OK) {
        esp_err_t err = s_scan_error;
        portEXIT_CRITICAL(&s_wifi_lock);
        return err;
    }
    if (capacity < s_scan_result_count) {
        portEXIT_CRITICAL(&s_wifi_lock);
        return ESP_ERR_INVALID_SIZE;
    }

    memcpy(access_points, s_scan_results, s_scan_result_count * sizeof(*access_points));
    *count = s_scan_result_count;
    portEXIT_CRITICAL(&s_wifi_lock);
    return ESP_OK;
}

esp_err_t app_wifi_station_connect(const char *ssid, const char *password)
{
    esp_err_t err = validate_credentials(ssid, password);
    if (err != ESP_OK) {
        return err;
    }

    portENTER_CRITICAL(&s_wifi_lock);
    if (!s_wifi_started) {
        portEXIT_CRITICAL(&s_wifi_lock);
        return ESP_ERR_INVALID_STATE;
    }
    bool disconnect_first =
        s_status.state == APP_WIFI_STATE_CONNECTED ||
        s_status.state == APP_WIFI_STATE_CONNECTING;
    memset(s_active_ssid, 0, sizeof(s_active_ssid));
    memset(s_active_password, 0, sizeof(s_active_password));
    memset(s_status.ssid, 0, sizeof(s_status.ssid));
    memcpy(s_active_ssid, ssid, strlen(ssid) + 1);
    memcpy(s_active_password, password, strlen(password) + 1);
    memcpy(s_status.ssid, ssid, strlen(ssid) + 1);
    s_status.state = APP_WIFI_STATE_CONNECTING;
    s_status.last_error = ESP_OK;
    s_status.disconnect_reason = 0;
    s_status.credentials_saved = false;
    s_status.web_server_ready = false;
    s_status.ipv4[0] = '\0';
    s_has_credentials = true;
    s_retry_count = 0;
    portEXIT_CRITICAL(&s_wifi_lock);

    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.sta.password, password, strlen(password));
    wifi_config.sta.threshold.authmode =
        password[0] == '\0' ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        set_connection_failed(err);
        return err;
    }

    if (disconnect_first) {
        err = esp_wifi_disconnect();
        if (err == ESP_ERR_WIFI_NOT_CONNECT) {
            err = esp_wifi_connect();
        }
        if (err != ESP_OK) {
            set_connection_failed(err);
            return err;
        }
        return ESP_OK;
    }

    err = esp_wifi_connect();
    if (err != ESP_OK) {
        set_connection_failed(err);
    }
    return err;
}

esp_err_t app_wifi_station_get_status(app_wifi_status_t *status)
{
    if (status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&s_wifi_lock);
    memcpy(status, &s_status, sizeof(*status));
    portEXIT_CRITICAL(&s_wifi_lock);
    return ESP_OK;
}
