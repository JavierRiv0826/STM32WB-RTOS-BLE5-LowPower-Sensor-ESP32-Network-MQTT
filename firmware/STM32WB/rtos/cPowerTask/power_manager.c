/*
 * power_manager.c
 *
 *  Created on: 23 mar 2026
 *      Author: Xavi
 */

#include "power_manager.h"
#include "app_ble.h"
#include "app_conf.h"
#include "stm32_lpm.h"
#include "cmsis_os.h"
#include "hw_if.h"
#include "main.h"
#include "sensor_task.h"

/* ───────────────────────────────────────────── */
/* Thread Flags                                 */
/* ───────────────────────────────────────────── */
#define PM_NOTIF_PERIODIC   (1U << 0)
#define PM_NOTIF_ADV_DONE   (1U << 1)

/* ───────────────────────────────────────────── */
/* Timing Configuration                         */
/* ───────────────────────────────────────────── */
/* 300 seconds (5 min) periodic wake */
//#define PM_PERIODIC_TICKS  ((uint32_t)(300UL * 1000000UL / CFG_TS_TICK_VAL))
#define PM_PERIODIC_TICKS  ((uint32_t)(60UL * 1000000UL / CFG_TS_TICK_VAL)) //60 seconds for  quick test

/* 30 second first sensor trigger */
#define PM_FIRST_TRIGGER   ((uint32_t)(30UL * 1000000UL / CFG_TS_TICK_VAL))

/* 10 second advertising window */
#define PM_ADV_TICKS       ((uint32_t)(10UL * 1000000UL / CFG_TS_TICK_VAL))

/* ───────────────────────────────────────────── */
/* Static Variables                             */
/* ───────────────────────────────────────────── */
static osThreadId_t  s_pm_task_handle = NULL;
static pm_state_t    s_state          = PM_STATE_ACTIVE;

static uint8_t       s_periodic_timer_id = 0xFF;
static uint8_t       s_first_tr_timer_id = 0xFF;
static uint8_t       s_adv_timer_id      = 0xFF;

/* ───────────────────────────────────────────── */
/* Timer Server Callbacks (ISR Context)         */
/* ───────────────────────────────────────────── */

static void periodic_timer_callback(void)
{
    if (s_pm_task_handle)
        osThreadFlagsSet(s_pm_task_handle, PM_NOTIF_PERIODIC);
    HAL_GPIO_TogglePin(GPIOA, LED_RTC_Pin);
}

static void first_trigger_timer_callback(void)
{
    if (s_pm_task_handle)
        osThreadFlagsSet(s_pm_task_handle, PM_NOTIF_PERIODIC);
    HAL_GPIO_TogglePin(GPIOA, LED_RTC_Pin);
}

static void adv_timer_callback(void)
{
    if (s_pm_task_handle)
        osThreadFlagsSet(s_pm_task_handle, PM_NOTIF_ADV_DONE);
    //HAL_GPIO_TogglePin(GPIOA, LED_INT_Pin);
}

/* ───────────────────────────────────────────── */
/* Public API                                   */
/* ───────────────────────────────────────────── */
pm_state_t PowerManager_GetState(void)
{
    return s_state;
}

/* ───────────────────────────────────────────── */
/* Power Task                                   */
/* ───────────────────────────────────────────── */

void PowerTask_Run(void *argument)
{
    (void)argument;

    s_pm_task_handle = osThreadGetId();

    /* Create periodic measurement timer (repeated) */
    HW_TS_Create(CFG_TIM_PROC_ID_POWER_MANAGER,
                 &s_periodic_timer_id,
                 hw_ts_Repeated,
                 periodic_timer_callback);

    /* Create first sensor trigger timer (one-shot) */
	HW_TS_Create(CFG_TIM_PROC_ID_POWER_MANAGER,
				 &s_first_tr_timer_id,
				 hw_ts_SingleShot,
				 first_trigger_timer_callback);

    /* Create advertising timeout timer (one-shot) */
    HW_TS_Create(CFG_TIM_PROC_ID_POWER_MANAGER,
                 &s_adv_timer_id,
				 hw_ts_SingleShot,
                 adv_timer_callback);

    /* Start periodic wake timer */
    HW_TS_Start(s_periodic_timer_id, PM_PERIODIC_TICKS);

    /*First sensor trigger timer*/
    HW_TS_Start(s_first_tr_timer_id, PM_FIRST_TRIGGER);

    /* Start advertising timeout */
    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);
    HW_TS_Start(s_adv_timer_id, PM_ADV_TICKS);

    /* Allow Stop2 */
    UTIL_LPM_SetStopMode(1 << CFG_LPM_APP, UTIL_LPM_ENABLE);

    for (;;)
    {
        /* ===================================== */
        /* Enter STOP2 when idle                 */
        /* ===================================== */
    	//HAL_GPIO_WritePin(GPIOA, LED_Stop_Pin, GPIO_PIN_SET);
        s_state = PM_STATE_STOP2;

        uint32_t flags = osThreadFlagsWait(
                PM_NOTIF_PERIODIC |
                PM_NOTIF_ADV_DONE,
                osFlagsWaitAny,
                osWaitForever);

        /* Wake occurred */
        HAL_GPIO_WritePin(GPIOA, LED_Stop_Pin, GPIO_PIN_RESET);
        UTIL_LPM_SetStopMode(1 << CFG_LPM_APP, UTIL_LPM_DISABLE);
        s_state = PM_STATE_ACTIVE;

        /* ===================================== */
        /* PERIODIC OR PIR WAKE                  */
        /* ===================================== */
        if (flags & (PM_NOTIF_PERIODIC))
        {
            s_state = PM_STATE_MEASURE;

            /* Trigger sensor measurement */
            SensorTask_TriggerMeasurement();

            s_state = PM_STATE_BLE_ADV;

            /* Start BLE advertising */
            //HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);
            //APP_BLE_Start_Advertising();
            /* Start advertising timeout */
            //HW_TS_Start(s_adv_timer_id, PM_ADV_TICKS);
        }

        /* ===================================== */
        /* ADVERTISING TIMEOUT                   */
        /* ===================================== */
        if (flags & PM_NOTIF_ADV_DONE)
        {
        	HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
			APP_BLE_Stop_Advertising();
        }

        /* Re-enable Stop2 for next idle */
        UTIL_LPM_SetStopMode(1 << CFG_LPM_APP, UTIL_LPM_ENABLE);
    }
}
