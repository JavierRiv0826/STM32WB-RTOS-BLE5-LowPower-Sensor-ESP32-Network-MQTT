#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "data_types.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "ble_central.h"
#include "oled_display.h"
#include "system_monitor.h"
#include "system_events.h"

static const char *TAG = "MAIN";

system_status_t g_system_status;
SemaphoreHandle_t status_mutex;
QueueHandle_t oled_event_queue;
QueueHandle_t sensor_queue;

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_LOGI(TAG, "BLE-WiFi Gateway Starting...");
    ESP_LOGI(TAG, "----------------------------");

    status_mutex = xSemaphoreCreateMutex();
    oled_event_queue = xQueueCreate(10, sizeof(oled_event_t));
    sensor_queue = xQueueCreate(10, sizeof(sensor_data_t));

    ble_central_init();
    wifi_manager_init();
    //nvs_flash_init();
    system_events_init();
    ESP_LOGI(TAG, "----------------------------");

    xTaskCreate(wifi_task,     "wifi_task", 4096, NULL, 5, NULL);
    xTaskCreate(mqtt_task,     "mqtt_task", 4096, NULL, 5, NULL);
    xTaskCreate(oled_task,     "oled_task", 4096, NULL, 3, NULL);
    xTaskCreate(watchdog_task, "wdt_task",  2048, NULL, 8, NULL);
    ESP_LOGI(TAG, "----------------------------");
}