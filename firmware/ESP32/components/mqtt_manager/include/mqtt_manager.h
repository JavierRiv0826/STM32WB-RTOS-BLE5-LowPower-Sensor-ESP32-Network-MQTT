#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

void mqtt_manager_init(void);
void mqtt_task(void *param);
void mqtt_publish_sensor(const char *topic, const char *payload);

#endif