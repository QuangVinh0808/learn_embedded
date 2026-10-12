#include "web_server.h"

#include <stdbool.h>
#include "lm35.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <stdio.h>
#include "ota_update.h"

static const char *tag = "[HTTP_SERVER]";

static httpd_handle_t server = NULL;

/* Handler: HTTP GET / */
static esp_err_t root_handler(httpd_req_t *req)
{
    const char *body =
        "hello world !\n"
        "Toi la Vinh !";

    esp_err_t err = httpd_resp_set_type(req,"text/plain; charset=utf-8");

    if (err != ESP_OK) return err;

    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}

/* Handler: HTTP GET /status */
static esp_err_t status_handler(httpd_req_t *req)
{
    const char *body =
        "{"
        "\"mode\":\"SoftAP\","
        "\"http_server\":\"running\""
        "}";

    esp_err_t err = httpd_resp_set_type(req, "application/json");

    if (err != ESP_OK) return err;

    return httpd_resp_send(req,body,HTTPD_RESP_USE_STRLEN );
}
static esp_err_t temp_handler(httpd_req_t *req)
{
    lm35_data_t data;
    esp_err_t err = lm35_read(&data);

    if (err != ESP_OK) {
        ESP_LOGE(tag, "LM35 read failed: %s", esp_err_to_name(err));
        return httpd_resp_send_500(req);
    }

    char body[128];
    int len = snprintf(
        body,
        sizeof(body),
        "{"
        "\"temperature_c\": %.2f,"
        "\"voltage_mv\": %u,"
        "\"raw\": %u"
        "}",
        data.temperature_c,
        data.voltage_mv,
        data.raw
    );

    if (len < 0 || len >= sizeof(body)) {
        ESP_LOGE(tag, "JSON formatting error");
        return httpd_resp_send_500(req);
    }

    err = httpd_resp_set_type(req, "application/json");

    if (err != ESP_OK) return err;

    return httpd_resp_send(req, body, len);
}
static const httpd_uri_t temp_route = {
    .uri = "/temp",
    .method = HTTP_GET,
    .handler = temp_handler,
    .user_ctx = NULL,
    .is_websocket = false,
};
static const httpd_uri_t root_route = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_handler,
    .user_ctx = NULL,
    .is_websocket = false,
};

static const httpd_uri_t status_route = {
    .uri = "/status",
    .method = HTTP_GET,
    .handler = status_handler,
    .user_ctx = NULL,
    .is_websocket = false,
};

esp_err_t http_server_start(void)
{
    if (server != NULL) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.server_port = 80;

    /* Thêm không gian stack cho handler OTA. */
    config.stack_size = 8192;

    /* Một lần chờ nhận dữ liệu có timeout 5 giây. */
    config.recv_wait_timeout = 5;

    esp_err_t err = httpd_start(
        &server,
        &config
    );

    if (err != ESP_OK) {
        ESP_LOGE(
            tag,
            "HTTP start failed: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    err = httpd_register_uri_handler(server, &root_route);

    if (err != ESP_OK) {
        goto register_failed;
    }

    err = httpd_register_uri_handler(server, &status_route);

    if (err != ESP_OK) {
        goto register_failed;
    }

    err = httpd_register_uri_handler(server, &temp_route);

    if (err != ESP_OK) {
        goto register_failed;
    }

    /* Bổ sung GET /ota và POST /ota. */
    err = ota_register_routes(server);

    if (err != ESP_OK) {
        goto register_failed;
    }

    ESP_LOGI(
        tag,
        "HTTP ready: /, /status, /temp, /ota"
    );

    return ESP_OK;

register_failed:
    ESP_LOGE(
        tag,
        "Register route failed: %s",
        esp_err_to_name(err)
    );

    httpd_stop(server);
    server = NULL;

    return err;
}