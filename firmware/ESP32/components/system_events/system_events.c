#include "system_events.h"

EventGroupHandle_t system_event_group = NULL;

void system_events_init(void)
{
    system_event_group = xEventGroupCreate();
}