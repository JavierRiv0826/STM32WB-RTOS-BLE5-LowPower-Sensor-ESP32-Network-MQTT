idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        wifi_manager
        mqtt_manager
        ble_central
        oled_display
        system_monitor
        system_events
        common
)