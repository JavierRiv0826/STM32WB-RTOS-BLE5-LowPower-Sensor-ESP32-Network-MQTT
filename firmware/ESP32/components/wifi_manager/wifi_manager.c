#include "wifi_manager.h"
#include "system_events.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WIFI";

static int retry_count = 0;
#define WIFI_MAX_RETRY 5

/* ========================= */
/*      EVENT HANDLER        */
/* ========================= */

static void wifi_event_handler(
    void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi STA Start");
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (retry_count < WIFI_MAX_RETRY)
        {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGI(TAG, "Retry WiFi connection...");
        }
        else
        {
            ESP_LOGE(TAG, "WiFi Failed after retries");
        }

        xEventGroupClearBits(system_event_group, WIFI_CONNECTED_BIT);
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        retry_count = 0;

        xEventGroupSetBits(system_event_group, WIFI_CONNECTED_BIT);
    }
}

/* ========================= */
/*      WIFI INIT            */
/* ========================= */

void wifi_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing WiFi...");

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL);

    esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "YOUR_SSID",
            .password = "YOUR_PASSWORD",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    /* 🔥 Disable power save (very important for BLE coexistence) */
    esp_wifi_set_ps(WIFI_PS_NONE);
}

/* ========================= */
/*         TASK              */
/* ========================= */

void wifi_task(void *pvParameters)
{
    ESP_LOGI(TAG, "WiFi Task Started");

    while (1)
    {
        /* Optional: monitor connection */
        EventBits_t bits = xEventGroupGetBits(system_event_group);

        if (bits & WIFI_CONNECTED_BIT)
        {
            ESP_LOGD(TAG, "WiFi Connected");
        }
        else
        {
            ESP_LOGD(TAG, "WiFi Not Connected");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}