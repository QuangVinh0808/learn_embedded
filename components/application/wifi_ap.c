#include <string.h>

#include "wifi_ap.h"
#include "app_config.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

static const char *tag = "[WIFI_AP]";

static esp_netif_t *ap_netif = NULL;

static void wifi_event_handler( void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;

    if (event_base != WIFI_EVENT) return;

    switch (event_id) {
        case WIFI_EVENT_AP_START:
            ESP_LOGI(tag, "SoftAP bat dau!");
            ESP_LOGI(tag, "SSID: %s", WIFI_SSID);
            ESP_LOGI(tag, "IP: 192.168.4.1");
            break;

        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
            ESP_LOGI( tag, "Station connected, AID=%d", event->aid);
            break;
        }

        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t *event =
                (wifi_event_ap_stadisconnected_t *)event_data;

            ESP_LOGI(
                tag,
                "Station disconnected, AID=%d",
                event->aid
            );

            break;
        }

        default:
            break;
    }
}

esp_err_t wifi_init_softap(void)
{
    esp_err_t err;

    err = esp_netif_init();
    if (err != ESP_OK) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK) return err;
    ap_netif = esp_netif_create_default_wifi_ap();
    if (ap_netif == NULL) {
        return ESP_FAIL;
    }
    /* Tạo Wi-Fi driver */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) return err;

    /* Đăng ký Wi-Fi event handler */
    err = esp_event_handler_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL
    );

    if (err != ESP_OK) return err;

    /* Cấu hình SoftAP */
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = WIFI_CHANNEL,
            .password = WIFI_PASSWORD,
            .max_connection = WIFI_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_wifi_set_config(
        WIFI_IF_AP,
        &wifi_config
    );

    if (err != ESP_OK) {
        return err;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(tag, "Wi-Fi SoftAP da duoc khoi tao!");

    return ESP_OK;
}

esp_netif_t *wifi_ap_get_netif(void)
{
    return ap_netif;
}