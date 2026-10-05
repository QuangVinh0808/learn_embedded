#pragma once

#include "esp_err.h"
#include "esp_netif.h"

esp_err_t wifi_init_softap(void);

esp_netif_t *wifi_ap_get_netif(void);