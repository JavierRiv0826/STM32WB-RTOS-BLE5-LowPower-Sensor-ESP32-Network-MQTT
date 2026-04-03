/*
 * aht20.c
 *
 *  Created on: 18 mar 2026
 *      Author: Xavi
 */

#include "aht20.h"

/* Private function prototypes -----------------------------------------------*/
static HAL_StatusTypeDef aht20_write_cmd(aht20_t *dev,
                                          uint8_t cmd,
                                          uint8_t p1, uint8_t p2);
static uint8_t aht20_crc8(const uint8_t *data, uint8_t len);

/* Public functions ----------------------------------------------------------*/

HAL_StatusTypeDef AHT20_Init(aht20_t *dev, I2C_HandleTypeDef *hi2c)
{
    dev->hi2c   = hi2c;
    dev->status = 0;

    HAL_Delay(40);  /* AHT20 power-on startup: ≥20 ms */

    /* Send soft reset first to guarantee a clean state */
    uint8_t reset_cmd = AHT20_CMD_SOFTRESET;
    HAL_I2C_Master_Transmit(dev->hi2c, AHT20_I2C_ADDR,
                            &reset_cmd, 1, 20);
    HAL_Delay(20);

    /* Send initialisation command */
    HAL_StatusTypeDef status =
        aht20_write_cmd(dev, AHT20_CMD_INIT, 0x08, 0x00);
    if (status != HAL_OK) return status;

    HAL_Delay(10);

    /* Verify calibrated bit is set in status byte */
    uint8_t stat = 0;
    status = HAL_I2C_Master_Receive(dev->hi2c, AHT20_I2C_ADDR,
                                    &stat, 1, 20);
    if (status != HAL_OK) return status;

    /* If calibration bit not set the sensor needs re-init */
    if (!(stat & AHT20_STATUS_CAL)) return HAL_ERROR;

    dev->status = stat;
    return HAL_OK;
}

HAL_StatusTypeDef AHT20_ReadData(aht20_t *dev, aht20_data_t *data)
{
    if (dev == NULL || data == NULL) return HAL_ERROR;

    /* Trigger measurement */
    HAL_StatusTypeDef status =
        aht20_write_cmd(dev, AHT20_CMD_TRIGGER, 0x33, 0x00);
    if (status != HAL_OK) return status;

    HAL_Delay(85);  /* conversion time: ≥80 ms per datasheet */

    /* Read 7 bytes: status(1) + data(5) + CRC(1) */
    uint8_t buf[7] = {0};
    status = HAL_I2C_Master_Receive(dev->hi2c, AHT20_I2C_ADDR,
                                    buf, 7, 100);
    if (status != HAL_OK) return status;

    dev->status = buf[0];

    /* Check busy bit */
    if (buf[0] & AHT20_STATUS_BUSY) return HAL_BUSY;

    /* Validate CRC-8 over first 6 bytes */
    if (aht20_crc8(buf, 6) != buf[6]) return HAL_ERROR;

    /* Extract 20-bit raw values */
    uint32_t raw_hum  = ((uint32_t)buf[1] << 12)
                      | ((uint32_t)buf[2] <<  4)
                      | ((buf[3] & 0xF0) >> 4);

    uint32_t raw_temp = ((uint32_t)(buf[3] & 0x0F) << 16)
                      | ((uint32_t)buf[4] <<  8)
                      |  (uint32_t)buf[5];

    /*
     * Integer compensation (no float):
     *   humidity_x100  = (raw_hum  * 10000) / 2^20
     *   temperature_x100 = (raw_temp * 20000) / 2^20 - 5000
     *
     * Use uint64_t for the intermediate multiply to avoid overflow:
     *   raw_hum max = 0xFFFFF = 1048575
     *   1048575 * 10000 = 10485750000 > UINT32_MAX → needs 64-bit
     */
    data->humidity_x100 =
        (uint16_t)((uint64_t)raw_hum * 10000ULL / 1048576ULL);

    data->temperature_x100 =
        (int32_t)((uint64_t)raw_temp * 20000ULL / 1048576ULL) - 5000;

    return HAL_OK;
}

/* Private functions ---------------------------------------------------------*/

static HAL_StatusTypeDef aht20_write_cmd(aht20_t *dev,
                                          uint8_t cmd,
                                          uint8_t p1, uint8_t p2)
{
    uint8_t buf[3] = {cmd, p1, p2};
    return HAL_I2C_Master_Transmit(dev->hi2c, AHT20_I2C_ADDR,
                                   buf, 3, 100);
}

/*
 * CRC-8 — polynomial 0x31, init value 0xFF.
 * Matches the AHT20 datasheet specification exactly.
 */
static uint8_t aht20_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
                crc = (uint8_t)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }
    return crc;
}

