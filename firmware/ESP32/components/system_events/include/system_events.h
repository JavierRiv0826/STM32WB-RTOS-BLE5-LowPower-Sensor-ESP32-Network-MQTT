#ifndef SYSTEM_EVENTS_H
#define SYSTEM_EVENTS_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/* Event bits */
#define WIFI_CONNECTED_BIT   BIT0
#define MQTT_CONNECTED_BIT   BIT1
#define BLE_CONNECTED_BIT    BIT2

extern EventGroupHandle_t system_event_group;

void system_events_init(void);

#endif