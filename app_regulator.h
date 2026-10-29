/**
 * @file temp_reg.h
 * @brief Модуль регулирования температуры теплоносителя.
 *
 * Модуль реализует логику управления нагревом с учётом:
 * - Аварийного режима (FAIL-SAFE) при потере датчика теплоносителя.
 * - Приоритетного внешнего комнатного термостата.
 * - Штатного регулирования с адаптацией тепловой инерции.
 * - Погодозависимого управления через weather_control.
 *
 * Модуль является синглтоном. Все состояния хранятся внутри статической
 * структуры TempRegulator_t, доступ к которой осуществляется через
 * temp_reg_instance().
 */

#ifndef TEMP_REG_H
#define TEMP_REG_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Состояния регулятора для индикации на экране или в веб-интерфейсе.
 */
typedef enum {
    REG_STATUS_FAILSAFE = -1, /**< Аварийный режим (ошибка датчика теплоносителя). */
    REG_STATUS_NORMAL = 0,    /**< Штатная работа по датчику теплоносителя. */
    REG_STATUS_THERMOSTAT     /**< Работа по приоритетному внешнему термостату. */
} RegStatus_t;

/**
 * @brief Снимок настроек, необходимый регулятору.
 *
 * t_outdoor вычисляется самим регулятором и в app_settings не хранится.
 */
typedef struct {
    float setting_temperature;
    float hysteresis;
    int32_t mode;
} reg_settings_snapshot_t;

RegStatus_t temp_reg_status(void);
esp_err_t app_regulator_start(void);



#endif // TEMP_REG_H
