/*
 * sensor_task.h
 *
 *  Created on: 19 mar 2026
 *      Author: Xavi
 */

#ifndef ASENSORTASK_SENSOR_TASK_H_
#define ASENSORTASK_SENSOR_TASK_H_

#include "cmsis_os.h"
#include "ble_service.h"

void SensorTask_Run(void *argument);
void SensorTask_TriggerMeasurement(void);

#endif /* 1ASENSORTASK_SENSOR_TASK_H_ */
