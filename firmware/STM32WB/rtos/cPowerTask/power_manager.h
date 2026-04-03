/*
 * power_manager.h
 *
 *  Created on: 23 mar 2026
 *      Author: Xavi
 */

#ifndef CPOWERTASK_POWER_MANAGER_H_
#define CPOWERTASK_POWER_MANAGER_H_

#include "cmsis_os.h"
#include <stdint.h>

/* ───────────────────────────────────────────── */
/* Power Manager States                         */
/* ───────────────────────────────────────────── */
typedef enum {
    PM_STATE_ACTIVE = 0,
    PM_STATE_MEASURE,
    PM_STATE_BLE_ADV,
    PM_STATE_BLE_CONN,
    PM_STATE_STOP2
} pm_state_t;

/* ───────────────────────────────────────────── */
/* ISR-safe notifications                       */
/* ───────────────────────────────────────────── */
/* Optional: allow other modules to request state */
pm_state_t PowerManager_GetState(void);

/* FreeRTOS task entry */
void PowerTask_Run(void *argument);

#endif /* CPOWERTASK_POWER_MANAGER_H_ */
