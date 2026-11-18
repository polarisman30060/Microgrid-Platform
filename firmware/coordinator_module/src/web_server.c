#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "web_server.h"

static const char *TAG = "web_server";
static const char INDEX_HTML[] =
    "<!doctype html>\n"
    "<html>\n"
    "<body>Hello Alicia</body>\n"
    "</html>\n";
static httpd_handle_t s_server;

static esp_err_t index_handler(httpd_req_t *request)
{
    ESP_RETURN_ON_ERROR(
        httpd_resp_set_type(request, "text/html; charset=utf-8"),
        TAG, "Could not set response content type");
    return httpd_resp_send(request, INDEX_HTML, sizeof(INDEX_HTML) - 1);
}

esp_err_t app_web_server_start(void)
{
    if (s_server != NULL) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
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
