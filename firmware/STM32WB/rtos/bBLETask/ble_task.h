/*
 * ble_task.h
 *
 *  Created on: 19 mar 2026
 *      Author: Xavi
 */

#ifndef BBLETASK_BLE_TASK_H_
#define BBLETASK_BLE_TASK_H_

#include "cmsis_os.h"
#include "sensor_task.h"

void BLETask_Run(void *argument);
void BLETask_NotifyMotion(void);
void BLETask_NotifyEnv(void);

#endif /* BBLETASK_BLE_TASK_H_ */
