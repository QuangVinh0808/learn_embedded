#ifndef __OTA_UPDATE_H__
#define __OTA_UPDATE_H__

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t ota_register_routes(httpd_handle_t server); // Dang ki get vaf post ota
esp_err_t ota_confirm_running_app(void);

#endif // __OTA_UPDATE_H__