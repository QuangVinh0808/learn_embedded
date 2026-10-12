idf_component_register(
    SRCS
        "main.c"
        "../components/application/wifi_ap.c"
        "../components/application/web_server.c"
        "../components/sensor/lm35.c"
        "../components/ota/ota_update.c"

    INCLUDE_DIRS
        "."
        "../lib"
        "../components/application"
        "../components/sensor"
        "../components/ota"

    REQUIRES
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        esp_http_server
        app_update
        esp_timer
)