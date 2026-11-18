#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "web_server.h"

#define WEB_MOUNT_POINT "/www"
#define WEB_PARTITION_LABEL "web_a"
#define WEB_INDEX_PATH WEB_MOUNT_POINT "/index.html"

static const char *TAG = "web_server";
static httpd_handle_t s_server;
static bool s_spiffs_mounted;

static void unmount_web_filesystem(void)
{
    esp_err_t err = esp_vfs_spiffs_unregister(WEB_PARTITION_LABEL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not unregister web partition: %s", esp_err_to_name(err));
    }
}

static esp_err_t mount_web_filesystem(void)
{
    if (s_spiffs_mounted) {
        return ESP_OK;
    }

    const esp_vfs_spiffs_conf_t config = {
        .base_path = WEB_MOUNT_POINT,
        .partition_label = WEB_PARTITION_LABEL,
        .max_files = 4,
        .format_if_mount_failed = false,
    };
    esp_err_t err = esp_vfs_spiffs_register(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not mount web partition: %s", esp_err_to_name(err));
        return err;
    }

    struct stat index_stat;
    if (stat(WEB_INDEX_PATH, &index_stat) != 0) {
        int stat_error = errno;
        ESP_LOGE(TAG, "Required web asset %s is unavailable: %s",
                 WEB_INDEX_PATH, strerror(stat_error));
        unmount_web_filesystem();
        return ESP_ERR_NOT_FOUND;
    }
    if (!S_ISREG(index_stat.st_mode)) {
        ESP_LOGE(TAG, "Required web asset %s is not a regular file", WEB_INDEX_PATH);
        unmount_web_filesystem();
        return ESP_ERR_NOT_FOUND;
    }

    s_spiffs_mounted = true;
    ESP_LOGI(TAG, "Mounted web filesystem at %s", WEB_MOUNT_POINT);
    return ESP_OK;
}

static esp_err_t index_handler(httpd_req_t *request)
{
    FILE *file = fopen(WEB_INDEX_PATH, "rb");
    if (file == NULL) {
        ESP_LOGE(TAG, "Could not open %s: %s", WEB_INDEX_PATH, strerror(errno));
        return httpd_resp_send_err(
            request, HTTPD_500_INTERNAL_SERVER_ERROR, "Dashboard asset unavailable");
    }

    esp_err_t err = httpd_resp_set_type(request, "text/html; charset=utf-8");
    if (err == ESP_OK) {
        err = httpd_resp_set_hdr(request, "Cache-Control", "no-cache");
    }
    if (err != ESP_OK) {
        fclose(file);
        return err;
    }

    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        err = httpd_resp_send_chunk(request, buffer, bytes_read);
        if (err != ESP_OK) {
            fclose(file);
            return err;
        }
    }

    if (ferror(file)) {
        ESP_LOGE(TAG, "Read failed while serving %s", WEB_INDEX_PATH);
        fclose(file);
        return ESP_FAIL;
    }
    fclose(file);
    return httpd_resp_send_chunk(request, NULL, 0);
}

esp_err_t app_web_server_start(void)
{
    if (s_server != NULL) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(mount_web_filesystem(), TAG, "Web filesystem unavailable");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.core_id = 0;
    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        return err;
    }

    const httpd_uri_t index_uri = {
        .uri = "/index.html",
        .method = HTTP_GET,
        .handler = index_handler,
    };
    err = httpd_register_uri_handler(s_server, &index_uri);
    if (err == ESP_OK) {
        const httpd_uri_t root_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = index_handler,
        };
        err = httpd_register_uri_handler(s_server, &root_uri);
    }

    if (err != ESP_OK) {
        esp_err_t stop_err = httpd_stop(s_server);
        s_server = NULL;
        if (stop_err != ESP_OK) {
            ESP_LOGE(TAG, "Could not stop HTTP server after setup failure: %s",
                     esp_err_to_name(stop_err));
        }
        return err;
    }

    ESP_LOGI(TAG, "HTTP server started");
    return ESP_OK;
}
