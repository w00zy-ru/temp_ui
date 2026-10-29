#pragma once
#ifndef APP_WIFI_H
#define APP_WIFI_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t wifi_manager_init(void);
bool wifi_is_connected(void);
void wifi_wait_connected(void);
void app_wifi_enable(void);
void app_wifi_disable(void);

// Функция для включения Wi-Fi (открывает семафор) для текущей сессии
void app_wifi_session_enable(void);
// Функция для выключения Wi-Fi (закрывает семафор) для текущей сессии
void app_wifi_session_disable(void);

#endif
