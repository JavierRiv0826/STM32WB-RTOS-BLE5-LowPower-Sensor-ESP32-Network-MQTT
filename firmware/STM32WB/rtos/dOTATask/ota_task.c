/*
 * ota_task.c
 *
 *  Created on: 19 mar 2026
 *      Author: Xavi
 */

#include "ota_task.h"
#include "cmsis_os.h"

void OTATask_Run(void *argument)
{
    (void)argument;
    for (;;)
    {
        /* OTA logic added Phase 8 */
        osDelay(osWaitForever);
    }
}
