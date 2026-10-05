#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "hw_hal.h"
#include "lm35.h"
#include "web_server.h"
#include "wifi_ap.h"

static const char *TAG = "[MAIN]";

static void gpio4_output_init(void)
{
    uint32_t value;

    //Enable clock
    value = read_reg(GPIO_CLOCK_GATE, 0xFFFFFFFFu);
    value |= 1u;
    write_reg(GPIO_CLOCK_GATE, value);

    // Config GPIO4 Output
    value = read_reg(IO_MUX_GPIO4, 0xFFFFFFFFu);
    value &= ~(IO_MUX_MCU_SEL_MASK |
               IO_MUX_FUN_DRV_MASK |
               IO_MUX_FUN_WPU |
               IO_MUX_FUN_WPD |
               IO_MUX_FUN_IE);
    value |= IO_MUX_MCU_SEL_GPIO | IO_MUX_FUN_DRV_20MA;
    write_reg(IO_MUX_GPIO4, value);

    /* GPIO_OUT[4] và GPIO_ENABLE[4] qua GPIO matrix. */
    value = read_reg(GPIO_FUNC4_OUT_SEL, 0xFFFFFFFFu);
    value &= ~(GPIO_FUNC_OUT_SEL_MASK |
               GPIO_FUNC_OUT_INV |
               GPIO_FUNC_OEN_SEL |
               GPIO_FUNC_OEN_INV);
    value |= GPIO_FUNC_OUT_SEL_GPIO | GPIO_FUNC_OEN_SEL;
    write_reg(GPIO_FUNC4_OUT_SEL, value);

    //Dùng push-pull mode
    value = read_reg(GPIO_PIN4, 0xFFFFFFFFu);
    value &= ~GPIO_PIN_PAD_DRIVER;
    write_reg(GPIO_PIN4, value);

    write_reg(GPIO_OUT_W1TC, LED_MASK);
    write_reg(GPIO_ENABLE_W1TS, LED_MASK);

    ESP_LOGI(TAG, "GPIO4 LED initialized");
}

static inline void gpio4_high(void)
{
    write_reg(GPIO_OUT_W1TS, LED_MASK);
}

static inline void gpio4_low(void)
{
    write_reg(GPIO_OUT_W1TC, LED_MASK);
}

static void nvs_init(void)
{
    esp_err_t err = nvs_flash_init();

    if ((err == ESP_ERR_NVS_NO_FREE_PAGES) ||
        (err == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "NVS initialized");
}

void app_main(void)
{
    nvs_init();
    gpio4_output_init();

    ESP_ERROR_CHECK(lm35_init());
    ESP_ERROR_CHECK(wifi_init_softap());
    ESP_ERROR_CHECK(http_server_start());

    ESP_LOGI(TAG, "Wi-Fi SSID: ESP32_Server");
    ESP_LOGI(TAG, "Open http://192.168.4.1/");
    ESP_LOGI(TAG, "Open http://192.168.4.1/status");

    while (1) {
        gpio4_high();
        vTaskDelay(pdMS_TO_TICKS(500));

        gpio4_low();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
