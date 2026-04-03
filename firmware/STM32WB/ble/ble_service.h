/*
 * ble_service.h
 *
 *  Created on: 19 mar 2026
 *      Author: Xavi
 */

#ifndef BLE_SERVICE_H_
#define BLE_SERVICE_H_

#include "stm32wbxx_hal.h"
#include "ble.h"
#include "tl.h"
#include <stdint.h>

/**
 * Sensor payload — built by sensor_task, consumed by BLE task.
 * Kept here so both tasks share the same definition.
 */
typedef struct {
    int32_t  temperature_x100;  /* AHT20  °C  × 100  e.g. 2452 = 24.52 °C */
    uint16_t humidity_x100;     /* AHT20  %RH × 100  e.g. 4360 = 43.60 %  */
    uint32_t pressure_pa;       /* BMP280 Pa         e.g. 78099           */
    uint8_t  battery_pct;       /* ADC    0-100                            */
} sensor_payload_t;

/* ── Public API ─────────────────────────────────────────────────────── */

/**
 * @brief  Register the Sensor Node GATT service with the BLE stack.
 *         Call once from APP_BLE_Init() inside USER CODE guard.
 */
void SensorService_Init(void);

/**
 * @brief  Push a full sensor reading to all subscribed clients.
 *         Call from BLE task after receiving from xSensorQueue.
 * @param  p  Pointer to payload — copied internally, caller can reuse.
 */
void SensorService_UpdateAll(const sensor_payload_t *p);

/**
 * @brief  Send a motion INDICATION (acknowledged by peer).
 *         Call from BLE task when motion_count > 0.
 * @param  count  Rolling event counter 0-255.
 */
void SensorService_IndicateMotion(uint8_t count);

/**
 * @brief  Returns 1 if a client has subscribed to notifications.
 *         Use to skip update calls when nobody is listening.
 */
uint8_t SensorService_IsNotifying(void);

/* Called directly from app_ble.c USER CODE guards */
void BLEService_SetConnected(uint16_t conn_handle);
void BLEService_SetDisconnected(void);

/* Add to ble_service.h public API section */
uint8_t BLEService_IsConnected(void);

#endif /* BLE_SERVICE_H_ */
