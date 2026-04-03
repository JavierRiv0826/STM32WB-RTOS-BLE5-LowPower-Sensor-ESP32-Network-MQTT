/*
 * ble_service.c
 *
 *  Created on: 19 mar 2026
 *      Author: Xavi
 */

/**
 * @file    sensor_service.c
 * @brief   Custom GATT service — Sensor Node
 *
 * Service UUID  0xA000  (private 16-bit)
 * ┌─ 0xA001  Temperature   int16  ×0.01°C   NOTIFY + READ
 * ├─ 0xA002  Humidity      uint16 ×0.01%RH  NOTIFY + READ
 * ├─ 0xA003  Pressure      uint32 Pa         NOTIFY + READ
 * ├─ 0xA004  Motion        uint8  count      INDICATE + READ
 * ├─ 0x2A19  Battery       uint8  0-100%     NOTIFY + READ
 * └─ 0xA005  OTA Control   uint8[]           WRITE + NOTIFY
 *
 * Lives entirely in this file — CubeMX never touches it.
 * app_ble.c calls SensorService_Init() once, inside a USER CODE guard.
 */

#include "ble_service.h"
#include "ble.h"
#include "dbg_trace.h"
#include "app_conf.h"

/* ── GATT handles ────────────────────────────────────────────────────── */
static uint16_t s_SvcHdle;          /* service                */
static uint16_t s_TempHdle;         /* 0xA001 temperature     */
static uint16_t s_HumHdle;          /* 0xA002 humidity        */
static uint16_t s_PressHdle;        /* 0xA003 pressure        */
static uint16_t s_MotionHdle;       /* 0xA004 motion          */
static uint16_t s_BattHdle;         /* 0x2A19 battery         */
static uint16_t s_OTAHdle;          /* 0xA005 OTA control     */

/* Tracks whether the client enabled notifications on temperature.
 * We use temperature CCCD as a proxy for "client is subscribed". */
static uint8_t  s_notifying = 0;

/* Connection handle — updated from SVCCTL event callback */
static uint16_t s_conn_handle = 0xFFFF;
static uint8_t  s_connected   = 0;

/* ── Forward declarations ────────────────────────────────────────────── */
static SVCCTL_EvtAckStatus_t SensorService_EventHandler(void *p_Event);

/* ── Helper: add one characteristic ─────────────────────────────────── */
static tBleStatus add_char(uint16_t svc_hdle,
                           uint16_t uuid,
                           uint8_t  size,
                           uint8_t  properties,
                           uint8_t  gatt_evt_mask,
                           uint8_t  is_variable,
                           uint16_t *out_hdle)
{
    return aci_gatt_add_char(svc_hdle,
                             UUID_TYPE_16,
                             (Char_UUID_t *)&uuid,
                             size,
                             properties,
                             ATTR_PERMISSION_NONE,
                             gatt_evt_mask,
                             10,
                             is_variable,
                             out_hdle);
}

/* ── Public API ──────────────────────────────────────────────────────── */

void SensorService_Init(void)
{
    tBleStatus ret;
    uint16_t   uuid;

    /* Register our event handler with the service controller */
    SVCCTL_RegisterSvcHandler(SensorService_EventHandler);

    /* ── Primary service 0xA000 ── */
    uuid = 0xA000;
    ret = aci_gatt_add_service(UUID_TYPE_16,
                               (Service_UUID_t *)&uuid,
                               PRIMARY_SERVICE,
                               20,          /* max attribute records:
                                               1 service
                                             + 2×char (decl+val) per char
                                             + 1 CCCD per notify/indicate char
                                               = 1 + 6×2 + 5×1 = 18 → use 20 */
                               &s_SvcHdle);
    if (ret != BLE_STATUS_SUCCESS)
    {
        APP_DBG_MSG("SensorService: add service FAILED 0x%02x\n", ret);
        return;
    }
    APP_DBG_MSG("SensorService: service handle 0x%04x\n", s_SvcHdle);

    /* Temperature 0xA001 — NOTIFY + READ, 2 bytes, fixed length */
    ret = add_char(s_SvcHdle, 0xA001, 2,
             CHAR_PROP_NOTIFY | CHAR_PROP_READ,
			 GATT_DONT_NOTIFY_EVENTS,
             CHAR_VALUE_LEN_CONSTANT,
             &s_TempHdle);
    if (ret != BLE_STATUS_SUCCESS)
    {
        APP_DBG_MSG("Char add failed: 0x%02x\n", ret);
        return;
    }

    /* Humidity 0xA002 — NOTIFY + READ, 2 bytes, fixed length */
    ret = add_char(s_SvcHdle, 0xA002, 2,
             CHAR_PROP_NOTIFY | CHAR_PROP_READ,
			 GATT_DONT_NOTIFY_EVENTS,
             CHAR_VALUE_LEN_CONSTANT,
             &s_HumHdle);
    if (ret != BLE_STATUS_SUCCESS)
    {
        APP_DBG_MSG("Char add failed: 0x%02x\n", ret);
        return;
    }

    /* Pressure 0xA003 — NOTIFY + READ, 4 bytes, fixed length */
    ret = add_char(s_SvcHdle, 0xA003, 4,
             CHAR_PROP_NOTIFY | CHAR_PROP_READ,
			 GATT_DONT_NOTIFY_EVENTS,
             CHAR_VALUE_LEN_CONSTANT,
             &s_PressHdle);
    if (ret != BLE_STATUS_SUCCESS)
    {
        APP_DBG_MSG("Char add failed: 0x%02x\n", ret);
        return;
    }

    /* Motion 0xA004 — INDICATE + READ, 1 byte, fixed length */
    ret = add_char(s_SvcHdle, 0xA004, 1,
             CHAR_PROP_INDICATE | CHAR_PROP_READ,
			 GATT_DONT_NOTIFY_EVENTS,
             CHAR_VALUE_LEN_CONSTANT,
             &s_MotionHdle);
    if (ret != BLE_STATUS_SUCCESS)
    {
        APP_DBG_MSG("Char add failed: 0x%02x\n", ret);
        return;
    }

    /* Battery 0x2A19 — NOTIFY + READ, 1 byte, fixed length */
    ret = add_char(s_SvcHdle, 0x2A19, 1,
             CHAR_PROP_NOTIFY | CHAR_PROP_READ,
             GATT_DONT_NOTIFY_EVENTS,
             CHAR_VALUE_LEN_CONSTANT,
             &s_BattHdle);
    if (ret != BLE_STATUS_SUCCESS)
    {
        APP_DBG_MSG("Char add failed: 0x%02x\n", ret);
        return;
    }

    /* OTA Control 0xA005 — WRITE + NOTIFY, up to 20 bytes, variable */
    ret = add_char(s_SvcHdle, 0xA005, 20,
             CHAR_PROP_WRITE | CHAR_PROP_NOTIFY,
             GATT_NOTIFY_ATTRIBUTE_WRITE,
             CHAR_VALUE_LEN_VARIABLE,
             &s_OTAHdle);
    if (ret != BLE_STATUS_SUCCESS)
    {
        APP_DBG_MSG("Char add failed: 0x%02x\n", ret);
        return;
    }

    /* Initialise all values to zero so READ before first measurement
     * returns a defined value rather than uninitialized memory */
    uint8_t zero[4] = {0, 0, 0, 0};
    aci_gatt_update_char_value(s_SvcHdle, s_TempHdle,   0, 2, zero);
    aci_gatt_update_char_value(s_SvcHdle, s_HumHdle,    0, 2, zero);
    aci_gatt_update_char_value(s_SvcHdle, s_PressHdle,  0, 4, zero);
    aci_gatt_update_char_value(s_SvcHdle, s_MotionHdle, 0, 1, zero);
    aci_gatt_update_char_value(s_SvcHdle, s_BattHdle,   0, 1, zero);

    APP_DBG_MSG("SensorService: init complete\n");
    APP_DBG_MSG("  Temp   handle: 0x%04x\n", s_TempHdle);
    APP_DBG_MSG("  Hum    handle: 0x%04x\n", s_HumHdle);
    APP_DBG_MSG("  Press  handle: 0x%04x\n", s_PressHdle);
    APP_DBG_MSG("  Motion handle: 0x%04x\n", s_MotionHdle);
    APP_DBG_MSG("  Batt   handle: 0x%04x\n", s_BattHdle);
    APP_DBG_MSG("  OTA    handle: 0x%04x\n", s_OTAHdle);
}

void SensorService_UpdateAll(const sensor_payload_t *p)
{
    if (p == NULL) return;

    uint8_t buf[4];

    /* Temperature — int16 little-endian */
    buf[0] = (uint8_t)( p->temperature_x100       & 0xFF);
    buf[1] = (uint8_t)((p->temperature_x100 >> 8) & 0xFF);
    aci_gatt_update_char_value(s_SvcHdle, s_TempHdle, 0, 2, buf);

    /* Humidity — uint16 little-endian */
    buf[0] = (uint8_t)( p->humidity_x100       & 0xFF);
    buf[1] = (uint8_t)((p->humidity_x100 >> 8) & 0xFF);
    aci_gatt_update_char_value(s_SvcHdle, s_HumHdle, 0, 2, buf);

    /* Pressure — uint32 little-endian */
    buf[0] = (uint8_t)( p->pressure_pa         & 0xFF);
    buf[1] = (uint8_t)((p->pressure_pa >>  8)  & 0xFF);
    buf[2] = (uint8_t)((p->pressure_pa >> 16)  & 0xFF);
    buf[3] = (uint8_t)((p->pressure_pa >> 24)  & 0xFF);
    aci_gatt_update_char_value(s_SvcHdle, s_PressHdle, 0, 4, buf);

    /* Battery — uint8 */
    aci_gatt_update_char_value(s_SvcHdle, s_BattHdle, 0, 1,
                               &p->battery_pct);

    APP_DBG_MSG("SensorService: updated T=%ld H=%u P=%lu Bat=%u\n",
                p->temperature_x100,
                p->humidity_x100,
                p->pressure_pa,
                p->battery_pct);
}

void SensorService_IndicateMotion(uint8_t count)
{
    aci_gatt_update_char_value(s_SvcHdle, s_MotionHdle, 0, 1, &count);
    APP_DBG_MSG("SensorService: motion indication count=%u\n", count);
}

uint8_t SensorService_IsNotifying(void)
{
    return s_notifying;
}

uint8_t BLEService_IsConnected(void)
{
    return s_connected;
}

void BLEService_SetConnected(uint16_t conn_handle)
{
    s_conn_handle = conn_handle;
    s_connected   = 1;
    s_notifying   = 0;
    APP_DBG_MSG("BLEService: connected handle=0x%04x\n", conn_handle);
}

void BLEService_SetDisconnected(void)
{
    s_conn_handle = 0xFFFF;
    s_connected   = 0;
    s_notifying   = 0;
    APP_DBG_MSG("BLEService: disconnected\n");
}

/* ── GATT event handler ──────────────────────────────────────────────── */

static SVCCTL_EvtAckStatus_t SensorService_EventHandler(void *p_Event)
{
    SVCCTL_EvtAckStatus_t  return_value = SVCCTL_EvtNotAck;
    hci_event_pckt        *p_event_pckt;
    evt_blecore_aci       *p_blecore_evt;
    aci_gatt_attribute_modified_event_rp0 *p_attribute_modified;

    p_event_pckt = (hci_event_pckt *)(((hci_uart_pckt *)p_Event)->data);

    if (p_event_pckt->evt != HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE)
        return SVCCTL_EvtNotAck;

    p_blecore_evt = (evt_blecore_aci *)p_event_pckt->data;

    switch (p_blecore_evt->ecode)
    {
        case ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE:
        {
            p_attribute_modified =
                (aci_gatt_attribute_modified_event_rp0 *)
                p_blecore_evt->data;

            if (p_attribute_modified->Attr_Handle == (s_TempHdle + 2U))
            {
                s_notifying =
                    (p_attribute_modified->Attr_Data[0] == 0x01U) ? 1U : 0U;
                APP_DBG_MSG("BLEService: notifications %s\n",
                            s_notifying ? "ON" : "OFF");
                return_value = SVCCTL_EvtAckFlowEnable;
            }
            else if (p_attribute_modified->Attr_Handle == s_OTAHdle)
            {
            	uint8_t cmd = p_attribute_modified->Attr_Data[0];
				APP_DBG_MSG("BLEService: OTA cmd=0x%02x\n", cmd);

				if (cmd == 0x01U)  /* OTA_CMD_START — reboot to OTA bootloader */
				{
					APP_DBG_MSG("BLEService: rebooting to OTA bootloader\n");

					/*
					 * The OTA bootloader at 0x08020000 checks for this magic value
					 * at a fixed address. If found, it enters OTA mode instead of
					 * passing control back to the application.
					 *
					 * MagicKeywordAddress points to MagicKeywordValue in flash.
					 * We write the reboot flag to SRAM so the bootloader finds it
					 * after reset — the bootloader reads from a known SRAM address.
					 */
					*((uint32_t *)SRAM1_BASE) = 0x94448A29U;

					/* Small delay so BLE stack can send acknowledgement */
					HAL_Delay(100);

					/* Reset — bootloader takes over */
					NVIC_SystemReset();
				}
				return_value = SVCCTL_EvtAckFlowEnable;
            }
            break;
        }

        default:
            break;
    }

    return return_value;
}
