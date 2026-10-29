#pragma once
#ifndef RC433_APP_RADIO_H
#define RC433_APP_RADIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_temperature.h"
#include "esp_err.h"
#include <stdint.h>

typedef struct {
    float temperature;    /**< Текущая температура, °C. */
    float volt;           /**< Текущее напряжение батареи, V. */
    float _hum;           /**< Пустое значение, пока не прииходит с внешнего датчика. */
    uint32_t last_update; /**< Тик FreeRTOS последнего успешного обновления. */
    uint8_t id;           /**< Идентификатор устройства 433mg */
    sensor_status_t status;
} radio433_data_t;

esp_err_t start_read_radio_data(void);

void app_rf433_switch_enable(bool enabled);

// Геттер просто отдает прямой адрес скрытого массива
const radio433_data_t *radio433_get_reference(void);
// Быстрая блокировка общего котла
void radio433_lock(void);
// Быстрая разблокировка
void radio433_unlock(void);

#define WITH_RADIO433_LOCK()                                                                       \
    for (bool _lock = (radio433_lock(), true); _lock; _lock = (radio433_unlock(), false))

#ifdef __cplusplus
}
#endif

#endif // RC433_APP_RADIO_H
