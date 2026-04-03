#include "mqtt_manager.h"
#include "system_events.h"

#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t mqtt_client = NULL;


void mqtt_publish_sensor(const char *topic, const char *payload)
{
    if (mqtt_client == NULL)
        return;

    if (!(xEventGroupGetBits(system_event_group) & MQTT_CONNECTED_BIT))
        return;

    esp_mqtt_client_publish(
        mqtt_client,
        topic,
        payload,
        0,
        1,
        0);
}


/* ========================= */
/*      MQTT EVENT HANDLER   */
/* ========================= */

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            xEventGroupSetBits(system_event_group, MQTT_CONNECTED_BIT);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT Disconnected");
            xEventGroupClearBits(system_event_group, MQTT_CONNECTED_BIT);
            break;

        default:
            break;
    }
}

/* ========================= */
/*          TASK             */
/* ========================= */

void mqtt_task(void *pvParameters)
{
    ESP_LOGI(TAG, "MQTT Task Started");

    while (1)
    {
        /* 1️⃣ Wait until WiFi is connected */
        ESP_LOGI(TAG, "Waiting for WiFi...");

        xEventGroupWaitBits(
            system_event_group,
            WIFI_CONNECTED_BIT,
            pdFALSE,
            pdTRUE,
            portMAX_DELAY);

        ESP_LOGI(TAG, "WiFi ready. Starting MQTT...");

        esp_mqtt_client_config_t mqtt_cfg = {
            .broker.address.uri = "mqtt://broker.hivemq.com",
        };

        mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

        esp_mqtt_client_register_event(
            mqtt_client,
            ESP_EVENT_ANY_ID,
            mqtt_event_handler,
            NULL);

        esp_mqtt_client_start(mqtt_client);

        /* 2️⃣ Wait until WiFi DISCONNECTS */
        while (xEventGroupGetBits(system_event_group) & WIFI_CONNECTED_BIT)
        {
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        ESP_LOGW(TAG, "WiFi lost. Stopping MQTT...");

        if (mqtt_client != NULL)
        {
            esp_mqtt_client_stop(mqtt_client);
            esp_mqtt_client_destroy(mqtt_client);
            mqtt_client = NULL;
        }
    }
}