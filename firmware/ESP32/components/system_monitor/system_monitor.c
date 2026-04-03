#include "system_monitor.h"
#include "esp_task_wdt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WDT";

void watchdog_task(void *param)
{
    ESP_LOGI(TAG, "Watchdog Task Started");
    esp_task_wdt_add(NULL);

    while (1)
    {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}