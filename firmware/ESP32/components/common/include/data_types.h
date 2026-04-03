#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float temperature;
    float humidity;
    float pressure;
    uint8_t motion;
    float battery;
    uint32_t timestamp;
} sensor_data_t;

typedef struct {
    bool wifi_connected;
    bool mqtt_connected;
    bool ble_connected;
    float last_temperature;
    uint8_t last_motion;
} system_status_t;

typedef enum {
    OLED_EVENT_WIFI_CHANGED,
    OLED_EVENT_MQTT_CHANGED,
    OLED_EVENT_BLE_CHANGED,
    OLED_EVENT_SENSOR_UPDATE
} oled_event_t;

#endif