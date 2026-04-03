/*
 * aht20.h
 *
 *  Created on: 18 mar 2026
 *      Author: Xavi
 */

#ifndef AHT20_AHT20_H_
#define AHT20_AHT20_H_

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal.h"   /* FIXED: was stm32f1xx */
#include <stdint.h>
#include <stdbool.h>

/* Defines -------------------------------------------------------------------*/
#define AHT20_I2C_ADDR      (0x38 << 1)

#define AHT20_CMD_INIT      0xBE
#define AHT20_CMD_TRIGGER   0xAC
#define AHT20_CMD_SOFTRESET 0xBA

#define AHT20_STATUS_BUSY   0x80
#define AHT20_STATUS_CAL    0x08   /* calibrated bit must be set after init */

/* Exported types ------------------------------------------------------------*/
typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint8_t            status;
} aht20_t;

typedef struct
{
    int32_t  temperature_x100;   /* °C  × 100  e.g. 2350 = 23.50 °C */
    uint16_t humidity_x100;      /* %RH × 100  e.g. 5500 = 55.00 %  */
} aht20_data_t;

/* Function prototypes -------------------------------------------------------*/
HAL_StatusTypeDef AHT20_Init(aht20_t *dev, I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef AHT20_ReadData(aht20_t *dev, aht20_data_t *data);

#endif /* AHT20_AHT20_H_ */
