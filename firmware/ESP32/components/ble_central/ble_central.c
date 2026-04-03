#include "ble_central.h"
#include "system_events.h"
#include "mqtt_manager.h"

#include <string.h>
#include <stdio.h>

#include "esp_log.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "host/util/util.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#define SENSOR_SERVICE_UUID   0xA000

static const char *TAG = "BLE";

/* ========================= */
/* Global State              */
/* ========================= */

static uint16_t g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint8_t  g_own_addr_type;
static bool     g_scanning = false;

static uint16_t svc_start = 0;
static uint16_t svc_end   = 0;

/* Characteristic value handles */
static uint16_t temp_handle   = 0;
static uint16_t hum_handle    = 0;
static uint16_t press_handle  = 0;
static uint16_t motion_handle = 0;
static uint16_t batt_handle   = 0;

/* CCCD sequential write control */
static uint16_t cccd_list[5];
static uint8_t  cccd_type[5];   // 1 = notify, 2 = indicate
static int cccd_count = 0;
static int cccd_index = 0;

/* ========================= */
/* Forward Declarations      */
/* ========================= */

static void ble_start_scan(void);
static int  ble_gap_event(struct ble_gap_event *event, void *arg);
static int  gatt_svc_cb(uint16_t conn_handle,
                        const struct ble_gatt_error *error,
                        const struct ble_gatt_svc *service,
                        void *arg);

static int  gatt_chr_cb(uint16_t conn_handle,
                        const struct ble_gatt_error *error,
                        const struct ble_gatt_chr *chr,
                        void *arg);

static int  cccd_write_cb(uint16_t conn_handle,
                          const struct ble_gatt_error *error,
                          struct ble_gatt_attr *attr,
                          void *arg);

static void ble_on_sync(void);
static void ble_host_task(void *param);

/* ========================= */

static void ble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/* ========================= */

static void ble_start_scan(void)
{
    if (g_scanning) return;

    struct ble_gap_disc_params params;
    memset(&params, 0, sizeof(params));

    params.passive = 0;
    params.itvl = 0x0010;
    params.window = 0x0010;
    params.filter_duplicates = 1;

    ESP_LOGI(TAG, "Starting scan...");

    if (ble_gap_disc(g_own_addr_type,
                     BLE_HS_FOREVER,
                     &params,
                     ble_gap_event,
                     NULL) == 0) {
        g_scanning = true;
    }
}

/* ========================= */
/* SERVICE DISCOVERY         */
/* ========================= */

static int gatt_svc_cb(uint16_t conn_handle,
                       const struct ble_gatt_error *error,
                       const struct ble_gatt_svc *service,
                       void *arg)
{
    if (error->status == 0) {

        if (ble_uuid_u16(&service->uuid.u) == SENSOR_SERVICE_UUID) {

            ESP_LOGI(TAG, ">>> FOUND CUSTOM SERVICE 0x%04X <<<",
                     SENSOR_SERVICE_UUID);

            svc_start = service->start_handle;
            svc_end   = service->end_handle;
        }
    }
    else if (error->status == BLE_HS_EDONE) {

        ESP_LOGI(TAG, "Service discovery complete");

        if (svc_start != 0) {
            ble_gattc_disc_all_chrs(conn_handle,
                                    svc_start,
                                    svc_end,
                                    gatt_chr_cb,
                                    NULL);
        }
    }

    return 0;
}

/* ========================= */
/* CHARACTERISTIC DISCOVERY  */
/* ========================= */

static int gatt_chr_cb(uint16_t conn_handle,
                       const struct ble_gatt_error *error,
                       const struct ble_gatt_chr *chr,
                       void *arg)
{
    if (error->status == 0) {

        uint16_t uuid = ble_uuid_u16(&chr->uuid.u);

        ESP_LOGI(TAG, "Char UUID=0x%04X handle=%d",
                 uuid, chr->val_handle);

        switch (uuid) {
            case 0xA001: temp_handle   = chr->val_handle; break;
            case 0xA002: hum_handle    = chr->val_handle; break;
            case 0xA003: press_handle  = chr->val_handle; break;
            case 0xA004: motion_handle = chr->val_handle; break;
            case 0x2A19: batt_handle   = chr->val_handle; break;
        }
    }
    else if (error->status == BLE_HS_EDONE) {

        ESP_LOGI(TAG, "Characteristic discovery complete");

        cccd_count = 0;
        cccd_index = 0;

        if (temp_handle) {
            cccd_list[cccd_count] = temp_handle + 1;
            cccd_type[cccd_count++] = 1;
        }

        if (hum_handle) {
            cccd_list[cccd_count] = hum_handle + 1;
            cccd_type[cccd_count++] = 1;
        }

        if (press_handle) {
            cccd_list[cccd_count] = press_handle + 1;
            cccd_type[cccd_count++] = 1;
        }

        if (batt_handle) {
            cccd_list[cccd_count] = batt_handle + 1;
            cccd_type[cccd_count++] = 1;
        }

        if (motion_handle) {
            cccd_list[cccd_count] = motion_handle + 1;
            cccd_type[cccd_count++] = 2;
        }

        if (cccd_count > 0) {

            uint16_t enable = (cccd_type[0] == 2) ? 0x0002 : 0x0001;

            ble_gattc_write_flat(conn_handle,
                                 cccd_list[0],
                                 &enable,
                                 sizeof(enable),
                                 cccd_write_cb,
                                 NULL);
        }
    }

    return 0;
}

/* ========================= */
/* CCCD WRITE CALLBACK       */
/* ========================= */

static int cccd_write_cb(uint16_t conn_handle,
                         const struct ble_gatt_error *error,
                         struct ble_gatt_attr *attr,
                         void *arg)
{
    if (error->status == 0) {
        ESP_LOGI(TAG, "CCCD enabled");
    } else {
        ESP_LOGE(TAG, "CCCD write failed: %d", error->status);
    }

    cccd_index++;

    if (cccd_index < cccd_count) {

        uint16_t enable = (cccd_type[cccd_index] == 2) ? 0x0002 : 0x0001;

        ble_gattc_write_flat(conn_handle,
                             cccd_list[cccd_index],
                             &enable,
                             sizeof(enable),
                             cccd_write_cb,
                             NULL);
    }

    return 0;
}

/* ========================= */
/* GAP EVENT HANDLER         */
/* ========================= */

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
        case BLE_GAP_EVENT_DISC:
        {
            struct ble_hs_adv_fields fields;

            if (ble_hs_adv_parse_fields(&fields,
                                        event->disc.data,
                                        event->disc.length_data) != 0)
                return 0;

            if (fields.name &&
                strstr((char*)fields.name, "STM32WB") != NULL)
            {
                ESP_LOGI(TAG, ">>> FOUND STM32 DEVICE <<<");

                ble_gap_disc_cancel();
                g_scanning = false;

                ble_gap_connect(g_own_addr_type,
                                &event->disc.addr,
                                30000,
                                NULL,
                                ble_gap_event,
                                NULL);
            }

            return 0;
        }

        case BLE_GAP_EVENT_CONNECT:
        {
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "Connected!");
                xEventGroupSetBits(system_event_group, BLE_CONNECTED_BIT);
                g_conn_handle = event->connect.conn_handle;

                ble_gattc_disc_all_svcs(g_conn_handle,
                                        gatt_svc_cb,
                                        NULL);
            } else {
                xEventGroupClearBits(system_event_group, BLE_CONNECTED_BIT);
                ble_start_scan();
            }
            return 0;
        }

        case BLE_GAP_EVENT_NOTIFY_RX:
        {
            uint16_t attr = event->notify_rx.attr_handle;
            struct os_mbuf *om = event->notify_rx.om;

            uint8_t data[8];
            int len = OS_MBUF_PKTLEN(om);
            os_mbuf_copydata(om, 0, len, data);

            if (attr == temp_handle && len >= 2) {
                int16_t raw = data[0] | (data[1] << 8);
                float temp = raw / 100.0f;
                ESP_LOGI(TAG, "Temp: %.2f C", temp);

                char msg[32];
                snprintf(msg, sizeof(msg), "%.2f", temp);
                mqtt_publish_sensor("gateway/sensor/temperature", msg);
            }
            else if (attr == hum_handle && len >= 2) {
                uint16_t raw = data[0] | (data[1] << 8);
                float hum = raw / 100.0f;
                ESP_LOGI(TAG, "Humidity: %.2f %%", hum);

                char msg[32];
                snprintf(msg, sizeof(msg), "%.2f", hum);
                mqtt_publish_sensor("gateway/sensor/humidity", msg);
            }
            else if (attr == press_handle && len >= 4) {
                uint32_t raw = data[0] |
                              (data[1] << 8) |
                              (data[2] << 16) |
                              (data[3] << 24);
                ESP_LOGI(TAG, "Pressure: %lu Pa", raw);

                char msg[32];
                snprintf(msg, sizeof(msg), "%lu", raw);
                mqtt_publish_sensor("gateway/sensor/pressure", msg);
            }
            else if (attr == motion_handle && len >= 1) {
                ESP_LOGI(TAG, "Motion: %d", data[0]);

                char msg[8];
                snprintf(msg, sizeof(msg), "%d", data[0]);
                mqtt_publish_sensor("gateway/sensor/motion", msg);
            }
            else if (attr == batt_handle && len >= 1) {
                ESP_LOGI(TAG, "Battery: %d %%", data[0]);

                char msg[8];
                snprintf(msg, sizeof(msg), "%d", data[0]);
                mqtt_publish_sensor("gateway/sensor/battery", msg);
            }

            return 0;
        }

        case BLE_GAP_EVENT_DISCONNECT:
        {
            ESP_LOGW(TAG, "Disconnected");
            xEventGroupClearBits(system_event_group, BLE_CONNECTED_BIT);

            g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            svc_start = svc_end = 0;
            temp_handle = hum_handle = press_handle = 0;
            motion_handle = batt_handle = 0;

            ble_start_scan();
            return 0;
        }

        default:
            return 0;
    }
}

/* ========================= */

static void ble_on_sync(void)
{
    ble_hs_id_infer_auto(0, &g_own_addr_type);

    uint8_t addr_val[6];
    ble_hs_id_copy_addr(g_own_addr_type, addr_val, NULL);

    ESP_LOGI(TAG,
             "BLE Address: %02X:%02X:%02X:%02X:%02X:%02X",
             addr_val[5], addr_val[4], addr_val[3],
             addr_val[2], addr_val[1], addr_val[0]);

    ble_start_scan();
}

void ble_central_init(void)
{
    ESP_LOGI(TAG, "BLE Central Init");

    nimble_port_init();
    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_hs_cfg.sync_cb = ble_on_sync;

    nimble_port_freertos_init(ble_host_task);
}