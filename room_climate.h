
#pragma once
#ifndef APP_AHT20_BME_HEADER_H
#define APP_AHT20_BME_HEADER_H

#include "app_temperature.h"
#include <esp_err.h>
#include <stdint.h>

/**
 * @brief Общая структура для каналов
 * (чтобы функции могли принимать её как универсальный тип)
 */
typedef struct {
    uint32_t last_update;
    sensor_status_t status;
} BaseChannel_t;

/**
 * @brief Итоговая структура, объединяющая два разных типа каналов
 *  Хранилище для дополнительных климатических датчиков
 * здесь датчик AHT20 + BME280
 * собираем здесь
 * именование: ch1 = aht20, ch2 = bme280
 */
typedef struct {
    // Первый канал: классический ClimateChannel_t
    struct {
        struct {
            float t; // Температура
            float h; // Влажность
        } value;
        uint32_t last_update;
        sensor_status_t status;
    } ch0;

    // Второй канал: с другими именами полей в Storage
    struct {
        struct {
            float t; // Другое имя
            float p; // Другое имя
        } value;
        uint32_t last_update;
        sensor_status_t status;
    } ch1;

} ClimateChannel_t;

esp_err_t app_climate_init(void);

// Быстрая блокировка
void app_mutex_climat_lock(void);
// Геттер просто отдает прямой адрес скрытого массива только для чтения
const ClimateChannel_t *app_climate_storage_get(void);

// Быстрая разблокировка
void app_mutex_climat_unlock(void);

#define WITH_CLIMAT_LOCK()                                                                         \
    for (bool _lock = (app_mutex_climat_lock(), true); _lock;                                      \
         _lock = (app_mutex_climat_unlock(), false))

#endif
