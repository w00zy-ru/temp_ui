/**
 * @file app_temperature.h
 * @brief Модуль опроса температуры через АЦП ADS1115 (4 канала NTC-датчиков).
 *
 * Модуль обеспечивает:
 * - Опрос 4 каналов ADS1115 с медианным и EMA-фильтрами.
 * - Преобразование сырых ADC-значений в температуру по формуле Стейнхарта.
 * - Потокобезопасный доступ к данным через мьютекс FreeRTOS.
 * - Маппинг абстрактных датчиков (труба, улица, комната, резерв) на физические
 * каналы ADC.
 */

#pragma once
#ifndef APP_ADS1115_TEMPERATURE_HEADER_H
#define APP_ADS1115_TEMPERATURE_HEADER_H

#include <esp_err.h>
#include <stdint.h>

/** Количество каналов ADS1115 */
#define ADS1115_CHANNELS_COUNT 4

/* Окно валидности уличной температуры */
#define IS_TEMP_OUTSIDE_OUT_OF_RANGE(t) ((t) < -60.0f || (t) > 60.0f)

/* Окно валидности температуры теплоносителя */
#define IS_TEMP_PRIMARY_OUT_OF_RANGE(t) ((t) < 0.0f || (t) > 90.0f)

/**
 * @brief Маппинг замещающих датчиков на выходные данные.
 */
typedef enum {
    REPLACE_CHANNEL_NONE = 0, /**< Датчик не изменен. */
    RF433_TEMP_CHANNEL,       /**< Датчик радиоканал. */
    AHTX_TEMP_CHANNEL,        /**< Датчик улицы — канал 1. */
    BMEX_TEMP_CHANNEL,        /**< Датчик комнаты — канал 2. */
    COUNT_REPLACE_CHANNEL     /**< счетчик */
} ReplaceNameMapping_t;

typedef enum {
    SENSOR_STATUS_OK = 0,
    SENSOR_STATUS_NOT_READY,
    SENSOR_STATUS_ERROR,
    SENSOR_STATUS_OUT_OF_RANGE,
} sensor_status_t;

/**
 * @brief Данные одного канала датчика.
 */
typedef struct {
    float temperature;            /**< Текущая температура, °C. */
    uint32_t last_update;         /**< Тик FreeRTOS последнего успешного обновления. */
    ReplaceNameMapping_t replace; /**< На какой датчик была произведена подмена. */
    sensor_status_t status;       /**< Флаги статуса ( замер не удался / датчик в аварии). */
} TemperatureChannel_t;

/**
 * @brief Глобальное хранилище данных всех каналов.
 */
typedef struct {
    TemperatureChannel_t channels[ADS1115_CHANNELS_COUNT]; /**< Массив данных каналов. */
} TemperatureStorage_t;

/**
 * @brief Маппинг абстрактных датчиков на физические каналы ADS1115.
 */
typedef enum {
    CH_INDEX_PIPE = 0,    /**< Датчик трубы отопления — канал 0. */
    CH_INDEX_OUTDOOR = 1, /**< Датчик улицы — канал 1. */
    CH_INDEX_ROOM = 2,    /**< Датчик комнаты — канал 2. */
    CH_INDEX_RESERVE = 3, /**< Резервный 4-й канал. */
} SensorMapping_t;

/** Имена датчиков (соответствуют порядку SensorMapping_t). */
// clang-format off
static const char* const s_sensor_names[ADS1115_CHANNELS_COUNT] = {
    [CH_INDEX_PIPE] = "Труба отопления",
    [CH_INDEX_OUTDOOR] = "Улица",
    [CH_INDEX_ROOM] = "Комната",
    [CH_INDEX_RESERVE] = "Резервный канал"
};
// clang-format on

/**
 * @brief Инициализация модуля температуры.
 *
 * Создаёт мьютекс, инициализирует ADS1115, запускает задачи опроса и симуляции.
 * @return ESP_OK при успешной инициализации, иначе код ошибки.
 */
esp_err_t app_temperature_init(void);

/**
 * @brief Потокобезопасное копирование всех данных температуры.
 *
 * @return копируем данные из пинг-понг буфер, default при таймауте мьютекса.
 */
TemperatureStorage_t temperature_get_snapshot(void);

/**
 * @brief Потокобезопасное чтение данных одного канала.
 *
 * @param[out] data Указатель на структуру для копирования данных канала. Не
 * должен быть NULL.
 * @param[in] channel Индекс канала из SensorMapping_t.
 * @return true если данные успешно скопированы, false при невалидном
 * указателе/канале или таймауте.
 */
bool temperature_get_from_channel(TemperatureChannel_t *data, SensorMapping_t channel);

// Геттер просто отдает прямой адрес скрытого массива
const TemperatureStorage_t *temperature_get_reference(void);
// Быстрая блокировка общего котла
void temperature_lock(void);
// Быстрая разблокировка
void temperature_unlock(void);

#define WITH_TEMPERATURE_LOCK()                                                                    \
    for (bool _lock = (temperature_lock(), true); _lock; _lock = (temperature_unlock(), false))

#endif /* APP_ADS1115_TEMPERATURE_HEADER_H */