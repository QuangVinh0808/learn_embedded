#ifndef __HW_HAL__
#define __HW_HAL__

#include <stdint.h>

#define write_reg(reg, value)   (*(volatile uint32_t *)(uintptr_t)(reg) = (uint32_t)(value))
#define read_reg(reg, mask)     ((*(volatile uint32_t *)(uintptr_t)(reg)) & (uint32_t)(mask))

#define GPIO_BASE                0x60004000u
#define IO_MUX_BASE              0x60009000u

//Thanh ghi GPIO ngoại vi
#define GPIO_OUT_W1TS       (GPIO_BASE + 0x0008u)
#define GPIO_OUT_W1TC       (GPIO_BASE + 0x000Cu)
#define GPIO_ENABLE_W1TS    (GPIO_BASE + 0x0024u)
#define GPIO_ENABLE_W1TC    (GPIO_BASE + 0x0028u)
#define GPIO_CLOCK_GATE     (GPIO_BASE + 0x062Cu)


#define GPIO_PIN4  (GPIO_BASE + 0x0074u + 4u * 4u)
#define GPIO_FUNC4_OUT_SEL (GPIO_BASE + 0x0554u + 4u * 4u)
#define IO_MUX_GPIO4 (IO_MUX_BASE + 0x0014u)

#define LED_GPIO    4u
#define LED_MASK    (1u << LED_GPIO)

/* Các trường của IO_MUX_GPIO4_REG */
#define IO_MUX_FUN_WPD           (1u << 7)
#define IO_MUX_FUN_WPU           (1u << 8)
#define IO_MUX_FUN_IE           (1u << 9)
#define IO_MUX_FUN_DRV_MASK     (3u << 10)
#define IO_MUX_MCU_SEL_MASK     (7u << 12)

/* MCU_SEL = 1: chọn chức năng GPIO */
#define IO_MUX_MCU_SEL_GPIO     (1u << 12)


#define IO_MUX_FUN_DRV_20MA     (2u << 10) // xuat 20mA ra chan GPIO4

/* Các trường trong GPIO_FUNCn_OUT_SEL_CFG_REG */
#define GPIO_FUNC_OUT_SEL_MASK  0x1FFu
#define GPIO_FUNC_OUT_SEL_GPIO  0x100u
#define GPIO_FUNC_OUT_INV       (1u << 9)
#define GPIO_FUNC_OEN_SEL       (1u << 10)
#define GPIO_FUNC_OEN_INV       (1u << 11)

/* GPIO_PINn_REG bit 2: chọn open-drain */
#define GPIO_PIN_PAD_DRIVER     (1u << 2)
//============================================
//SENSOR LM35
//============================================
#define LM35_GPIO                   5u
#define LM35_GPIO_MASK              (1u << LM35_GPIO)
#define LM35_ADC_CHANNEL            4u
#define IO_MUX_GPIO5                (IO_MUX_BASE + 0x0018u)
#define SENS_RTC_BASE               (0x60008000u + 0x0800u)

#define SENS_SAR_READER1_CTRL       (SENS_RTC_BASE + 0x0000u)
#define SENS_SAR_MEAS1_CTRL2        (SENS_RTC_BASE + 0x000Cu)
#define SENS_SAR_MEAS1_MUX          (SENS_RTC_BASE + 0x0010u)
#define SENS_SAR_ATTEN1             (SENS_RTC_BASE + 0x0014u)

/* SENS_SAR_READER1_CTRL_REG */
#define SENS_SAR1_CLK_DIV_MASK      0xFFu
#define SENS_SAR1_CLK_DIV_VALUE     1u
#define SENS_SAR1_DATA_INV          (1u << 28)

/* SENS_SAR_MEAS1_CTRL2_REG */
#define SENS_SAR1_EN_PAD_FORCE      (1u << 31)
#define SENS_SAR1_EN_PAD_MASK       (0x0FFFu << 19)
#define SENS_MEAS1_START_FORCE      (1u << 18)
#define SENS_MEAS1_START_SAR        (1u << 17)
#define SENS_MEAS1_DONE_SAR         (1u << 16)
#define SENS_MEAS1_DATA_MASK        0xFFFFu

/* ESP32-S3 ADC trả kết quả 12 bit */
#define ADC1_RAW_12BIT_MASK         0x0FFFu

/*
 * ADC1 channel 4 tương ứng bit:
 * SENS_SAR1_EN_PAD[4] nằm tại bit 19 + 4 = 23
 */
#define LM35_ADC_CHANNEL_PAD        (1u << (19u + LM35_ADC_CHANNEL))

/* SENS_SAR_MEAS1_MUX_REG */
#define SENS_SAR1_DIG_FORCE         (1u << 31)

/*
 * Mỗi ADC channel dùng 2 bit attenuation.
 * ADC1 channel 4 nằm tại bit [9:8].
 */
#define LM35_ADC_ATTEN_SHIFT        (LM35_ADC_CHANNEL * 2u)

#define LM35_ADC_ATTEN_MASK         (3u << LM35_ADC_ATTEN_SHIFT)

#define ADC_ATTEN_0DB               0u
#endif /*__HW_HAL__*/
