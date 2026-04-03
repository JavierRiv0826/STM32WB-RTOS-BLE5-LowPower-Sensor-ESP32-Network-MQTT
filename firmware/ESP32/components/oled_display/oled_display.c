#include "oled_display.h"
#include "data_types.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "sh1106.h"
#include "system_events.h"

extern system_status_t g_system_status;
extern SemaphoreHandle_t status_mutex;
extern QueueHandle_t oled_event_queue;

static const char *TAG = "OLED";

void oled_task(void *param)
{
    ESP_LOGI(TAG, "oled Task Started");

    sh1106_init();

    // Static header (printed once)
    sh1106_draw_string(0, "Welcome");
    sh1106_draw_string(1, "BLE-WiFi Gateway");
    sh1106_update();

    while (1)
    {
        EventBits_t bits = xEventGroupGetBits(system_event_group);

        /* -------- WIFI STATUS -------- */
        if (bits & WIFI_CONNECTED_BIT)
        {
            if (bits & MQTT_CONNECTED_BIT)
                sh1106_draw_string(3, "WiFi+MQTT OK");
            else
                sh1106_draw_string(3, "WiFi OK");
        }
        else
        {
            sh1106_draw_string(3, "Connecting WiFi");
        }

        /* -------- BLE STATUS -------- */
        if (bits & BLE_CONNECTED_BIT)
        {
            sh1106_draw_string(5, "Sensor: Connected OK ");
        }
        else
        {
            sh1106_draw_string(5, "Sensor: Connecting... ");
        }

        sh1106_update();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}