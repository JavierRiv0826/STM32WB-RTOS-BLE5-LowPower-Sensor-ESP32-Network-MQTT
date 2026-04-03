/*
 * sensor_task.c
 *
 *  Created on: 19 mar 2026
 *      Author: Xavi
 */
/* Includes ------------------------------------------------------------------*/
#include "sensor_task.h"
#include "bmp280.h"
#include "aht20.h"
#include "main.h"
#include "i2c.h"
#include "adc.h"
#include "cmsis_os.h"
#include "ble_task.h"

/* Private defines -----------------------------------------------------------*/
/* Notification flags */
#define SENSOR_NOTIF_MEASURE  0x01U

/* Private  types  -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static bmp280_t      s_bmp;
static aht20_t       s_aht;

static osThreadId_t s_sensor_task_handle = NULL;

/* External variables --------------------------------------------------------*/
extern osMessageQueueId_t xSensorToBleQueueHandle;

/* Private function prototypes -----------------------------------------------*/
static uint8_t read_battery_percent(void);

/* Public functions ----------------------------------------------------------*/
void SensorTask_TriggerMeasurement(void)
{
    if (s_sensor_task_handle == NULL) return;
    osThreadFlagsSet(s_sensor_task_handle, SENSOR_NOTIF_MEASURE);
}

void SensorTask_Run(void *argument)
{
    (void)argument;
    s_sensor_task_handle = osThreadGetId();

    BMP280_Init(&s_bmp, &hi2c1);
    AHT20_Init(&s_aht, &hi2c1);

    for (;;)
    {
    	osThreadFlagsWait(SENSOR_NOTIF_MEASURE,
    	                  osFlagsWaitAny,
    	                  osWaitForever);

        /* ── Measure BMP280 ── */
        bmp280_data_t bmp_data = {0};
        BMP280_ConfigureForced(&s_bmp);
        osDelay(10);
        BMP280_ReadData(&s_bmp, &bmp_data);

        /* ── Measure AHT20 ── */
        aht20_data_t aht_data = {0};
        AHT20_ReadData(&s_aht, &aht_data);

        /* ── Build payload ── */
        sensor_payload_t payload = {
            .temperature_x100 = aht_data.temperature_x100,
            .humidity_x100    = (uint16_t)aht_data.humidity_x100,
            .pressure_pa      = bmp_data.pressure_pa,
            .battery_pct      = read_battery_percent()
        };

        if (osMessageQueuePut(xSensorToBleQueueHandle,
                              &payload,
                              0,
                              0) == osOK)
        {
            BLETask_NotifyEnv();
        }
        //HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
        //HAL_GPIO_TogglePin(GPIOA, LED_RTC_Pin);
        //HAL_GPIO_TogglePin(GPIOA, LED_INT_Pin);
        //HAL_GPIO_TogglePin(GPIOA, LED_Stop_Pin);
    }
}

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  Read battery percentage via ADC IN16 (Vbat ÷3 internal path).
 *         On the WeAct board powered by USB this will always read ~100%.
 *         On final hardware with CR2477 direct to 3V3 it reads correctly.
 */
static uint8_t read_battery_percent(void)
{
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 20) != HAL_OK)
    {
        HAL_ADC_Stop(&hadc1);
        return 0;
    }

    uint32_t raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    /* Vbat channel has internal ÷3 divider.
     * Vref = 3000 mV (no internal reference on WB55 without VREFBUF).
     * actual_mv = raw * 3000 * 3 / 4096  */
    uint32_t vbat_mv = (raw * 9000UL) / 4096UL;

    /* Clamp and convert to 0–100 % */
    if (vbat_mv >= 3000) return 100;
    if (vbat_mv <= 2000) return 0;
    return (uint8_t)((vbat_mv - 2000) * 100 / 1000);
}
