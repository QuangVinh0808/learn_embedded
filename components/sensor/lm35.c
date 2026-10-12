#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"

#include "hw_hal.h"
#include "lm35.h"

#define LM35_SAMPLE_COUNT       32u
#define ADC_MAX_VALUE           4095u
#define ADC_REFERENCE_MV        1100u
#define LM35_MV_PER_CELSIUS     10.0f
#define ADC_READ_TIMEOUT        100000u

static const char *TAG = "[LM35]";

static esp_err_t adc1_read_raw(uint16_t *raw)
{
    uint32_t value;
    uint32_t timeout = ADC_READ_TIMEOUT;

    if (raw == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    value = read_reg(SENS_SAR_MEAS1_CTRL2,0xFFFFFFFFu); 
    value &= ~SENS_MEAS1_START_SAR; // Đưa về 0

    write_reg(SENS_SAR_MEAS1_CTRL2,value);

    value |= SENS_MEAS1_START_SAR;

    write_reg(SENS_SAR_MEAS1_CTRL2,value);
    while (timeout > 0u)
    {
        value = read_reg(SENS_SAR_MEAS1_CTRL2,0xFFFFFFFFu);

        if ((value & SENS_MEAS1_DONE_SAR) != 0u)
        {

            *raw = (uint16_t)(value & ADC1_RAW_12BIT_MASK);
            return ESP_OK;
        }

        timeout--;
    }
    ESP_LOGE(TAG, "ADC conversion timeout");
    return ESP_ERR_TIMEOUT;
}


esp_err_t lm35_init(void)
{
    uint32_t value;
    write_reg(GPIO_OUT_W1TC,LM35_GPIO_MASK);

    write_reg(GPIO_ENABLE_W1TC,LM35_GPIO_MASK);

    value = read_reg(
        IO_MUX_GPIO5,
        0xFFFFFFFFu
    );
    value &= ~(IO_MUX_MCU_SEL_MASK |
               IO_MUX_FUN_IE |
               IO_MUX_FUN_WPU |
               IO_MUX_FUN_WPD);
    write_reg(IO_MUX_GPIO5,value);

    //Setup clock cho ADC1
    value = read_reg(
        SENS_SAR_READER1_CTRL,
        0xFFFFFFFFu
    );
    value &= ~(SENS_SAR1_CLK_DIV_MASK |
               SENS_SAR1_DATA_INV);
    value |= SENS_SAR1_CLK_DIV_VALUE;
    write_reg(SENS_SAR_READER1_CTRL,value);
    /*
     * Chọn RTC controller điều khiển ADC1.
     *
     * DIG_FORCE = 0:
     *      ADC không bị digital controller điều khiển.
     */
    value = read_reg(
        SENS_SAR_MEAS1_MUX,
        0xFFFFFFFFu
    );

    value &= ~SENS_SAR1_DIG_FORCE;

    write_reg(
        SENS_SAR_MEAS1_MUX,
        value
    );

    /*
     * Cấu hình attenuation cho ADC1 channel 4.
     */
    value = read_reg(
        SENS_SAR_ATTEN1,
        0xFFFFFFFFu
    );

    value &= ~LM35_ADC_ATTEN_MASK;

    value |= (
        ADC_ATTEN_0DB << LM35_ADC_ATTEN_SHIFT
    );

    write_reg(
        SENS_SAR_ATTEN1,
        value
    );

    value = read_reg(
        SENS_SAR_MEAS1_CTRL2,
        0xFFFFFFFFu
    );

    value &= ~(SENS_SAR1_EN_PAD_MASK |
               SENS_MEAS1_START_SAR);

    value |= SENS_SAR1_EN_PAD_FORCE |
             SENS_MEAS1_START_FORCE |
             LM35_ADC_CHANNEL_PAD;

    write_reg(
        SENS_SAR_MEAS1_CTRL2,
        value
    );

    ESP_LOGI(
        TAG,
        "LM35 initialized: GPIO%u, ADC1 channel %u",
        (unsigned int)LM35_GPIO,
        (unsigned int)LM35_ADC_CHANNEL
    );

    return ESP_OK;
}

esp_err_t lm35_read(lm35_data_t *data)
{
    uint32_t raw_sum = 0u;
    uint16_t raw_sample;
    uint16_t raw_average;
    uint16_t voltage_mv;

    if (data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    for (uint32_t i = 0u; i < LM35_SAMPLE_COUNT; i++)
    {
        esp_err_t err = adc1_read_raw(&raw_sample);

        if (err != ESP_OK)
        {
            return err;
        }

        raw_sum += raw_sample;
    }

    raw_average = (uint16_t)(
        raw_sum / LM35_SAMPLE_COUNT
    );

    voltage_mv = (uint16_t)(
        ((uint32_t)raw_average * ADC_REFERENCE_MV) /
        ADC_MAX_VALUE
    );

    data->raw = raw_average;
    data->voltage_mv = voltage_mv;
    data->temperature_c =
        (float)voltage_mv / LM35_MV_PER_CELSIUS;

    return ESP_OK;
}