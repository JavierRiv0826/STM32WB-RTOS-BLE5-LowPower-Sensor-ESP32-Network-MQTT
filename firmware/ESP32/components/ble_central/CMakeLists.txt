idf_component_register(
    SRCS "ble_central.c"
    INCLUDE_DIRS "include"
    REQUIRES mqtt_manager system_events
    PRIV_REQUIRES bt
)