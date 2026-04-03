idf_component_register(
    SRCS "oled_display.c"
    INCLUDE_DIRS "include"
    REQUIRES
        sh1106
    PRIV_REQUIRES
        common
        system_events
)