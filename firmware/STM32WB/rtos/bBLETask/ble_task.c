/*
 * ble_task.c
 *
 *  Created on: 19 mar 2026
 *      Author: Xavi
 */

/* Includes ------------------------------------------------------------------*/
#include "ble_task.h"
#include "ble_service.h"
#include "app_ble.h"
#include "cmsis_os.h"
#include "main.h"

/* Private defines -----------------------------------------------------------*/
#define BLE_NOTIF_MOTION   0x01U
#define BLE_NOTIF_ENV      0x02U
/* Private  types  -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static osThreadId_t s_ble_task_handle = NULL;
static uint8_t       s_motion_count = 0;

/* External variables --------------------------------------------------------*/
extern osMessageQueueId_t xSensorToBleQueueHandle;

/* Private function prototypes -----------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
void BLETask_NotifyMotion(void)
{
    if (s_ble_task_handle != NULL)
        osThreadFlagsSet(s_ble_task_handle, BLE_NOTIF_MOTION);
    if (s_motion_count < 255) s_motion_count++;
    HAL_GPIO_TogglePin(GPIOA, LED_INT_Pin);
}

void BLETask_NotifyEnv(void)
{
    if (s_ble_task_handle != NULL)
        osThreadFlagsSet(s_ble_task_handle, BLE_NOTIF_ENV);
    s_motion_count = 0;
}

void BLETask_Run(void *argument)
{
    (void)argument;
    s_ble_task_handle = osThreadGetId();
    sensor_payload_t payload;

    for (;;)
    {
    	uint32_t flags = osThreadFlagsWait(
							BLE_NOTIF_MOTION | BLE_NOTIF_ENV,
							osFlagsWaitAny,
							osWaitForever);

		if (flags & BLE_NOTIF_MOTION)
		{
			SensorService_IndicateMotion(s_motion_count);
		}

		if (flags & BLE_NOTIF_ENV)
		{
			while (osMessageQueueGet(xSensorToBleQueueHandle,
									 &payload,
									 NULL,
									 0) == osOK)
			{
				SensorService_UpdateAll(&payload);
			}
		}
    }
}

/* Private functions ---------------------------------------------------------*/

