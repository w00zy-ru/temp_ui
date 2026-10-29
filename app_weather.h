#ifndef WEATHER_CONTROL_H
#define WEATHER_CONTROL_H

#include "app_config.h"
#include <stdbool.h>
#include <stdint.h>
#include "app_regulator.h"

// Структура точки температурного графика (улица -> теплоноситель)
typedef struct {
    float sensor_temp; // Температура на улице
    float heater_temp; // Необходимая температура теплоносителя (t_low)
} table_map_t;

/**
 * @brief Неполное объявление типа (Incomplete Type).
 * Другие модули видят это имя, но не знают, какие внутри поля.
 */
typedef struct WeatherController_t WeatherController_t;

/**
 * @brief Инициализация погодозависимого блока
 * @param self Указатель на структуру блока
 */
WeatherController_t *weather_instance(const reg_settings_snapshot_t settings);

/**
 * @brief Расчет текущей установки по уличной температуре и передача её в app_config
 * @note Вызывать циклически в FreeRTOS задаче погодозависимой автоматики
 * @param self Указатель на структуру блока
 * @param current_outdoor_temp Текущая температура на улице с датчика
 * @param current_time_sec Системное время в секундах
 */
void weather_process(WeatherController_t *self, float *turn_on_target, float current_outdoor_temp,
                     uint32_t current_time_sec);

/**
 * @brief Функция долгосрочного самообучения (адаптации графика).
 * @note Вызывать реже (например, раз в 10 минут), передавая средние значения температур за этот
 * период
 * @param self Указатель на структуру блока
 * @param avg_outdoor Средняя температура на улице за расчетный период
 * @param avg_room Средняя температура в помещении за расчетный период
 * @param current_time_sec Системное время в секундах
 */
void weather_adapt_table(WeatherController_t *self, enum_reg_mode regulator_mode, float avg_outdoor,
                         float avg_room, uint32_t current_time_sec);

//  Костыль очистки времени ожидания рассчета табличной температуры
// нужно если режимы переключаются пересчитывать сразу

void weather_process_clear_adapt_timer(WeatherController_t *self, enum_reg_mode regulator_mode,
                                       uint32_t current_time_sec);

/*
 * Между точками используется линейная интерполяция.
 * Выше +5 °C возвращается 3 °C, ниже −30 °C — 7 °C.
 * Возвращаемое значение — float, чтобы интерполяция не теряла дробную часть.
 */
float get_hysteresis(float outdoor_temp);

#endif // WEATHER_CONTROL_H
