#ifndef APP_EVENTS_H
#define APP_EVENTS_H

#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

// Объявляем глобальную базу событий для нашего котла
ESP_EVENT_DECLARE_BASE(BOILER_EVENTS);
//  Перечисление всех событий (идентификаторы)
typedef enum {
    BOILER_EVENT_CONFIG_UPDATED,
    BOILER_EVENT_STATUS_UPDATED,
    BOILER_EVENT_TEMPS_UPDATED,

    // === СОБЫТИЯ WI-FI ===
    BOILER_EVENT_WIFI_CONNECTED,    // Wi-Fi успешно подключился, получен IP
    BOILER_EVENT_WIFI_DISCONNECTED, // Произошел обрыв связи
    BOILER_EVENT_WIFI_DISABLE,
    BOILER_EVENT_WIFI_PORTAL_START // Запущен веб-портал первоначальной настройки
} boiler_event_id_t;

#endif // APP_EVENTS_H_
