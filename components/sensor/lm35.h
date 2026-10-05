#ifndef LM35_H
#define LM35_H

#include <stdint.h>
#include "esp_err.h"
typedef struct
{
    uint16_t raw;
    uint16_t voltage_mv;
    float temperature_c;
} lm35_data_t;

esp_err_t lm35_init(void);
esp_err_t lm35_read(lm35_data_t *data);
#endif /* LM35_H */