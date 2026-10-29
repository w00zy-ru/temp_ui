/**
 * @file app_config.h
 * @brief Модуль управления настройками и статусом котла с сохранением в NVS.
 *
 * Модуль обеспечивает:
 * - Хранение и загрузку настроек (целевая температура, гистерезис, режим
 * работы) из/в NVS.
 * - Хранение и загрузку погодной таблицы (зависимость установки от наружной
 * температуры) из/в NVS.
 * - Потокобезопасное управление динамическим статусом системы (overshoot_correction,
 * relay, thermostat, wifi).
 * - Уведомление задачи реле через xTaskNotify при изменении состояния relay.
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Режимы работы регулятора температуры.
 */
typedef enum {
    MODE_HARDWARE_FAULT = -2, /**< Аварийный режим — всем стоп. */
    MODE_EMERGENCY = -1,      /**< Аварийный режим — циклический нагрев без датчика трубы. */
    MODE_MANUAL = 0,          /**< Ручной режим — оператор управляет реле напрямую. */
    MODE_WEATHER_ONLY,        /**< Погодозависимое управление по таблице. */
    MODE_WEATHER_ADAPTIVE,    /**< Погодозависимое с адаптивной коррекцией . */
    MODE_WEATHER_ROOM         /**< Погодозависимое с коррекцией по температуре комнаты. */
} enum_reg_mode;

// Текущая версия структуры. Если изменить поля, то
// надо изменить значение, и код сам поймет, что старый NVS нужно сбросить.
#define BOILER_CONFIG_VERSION 200

/**
 * @brief Настраиваемые и сохраняемые в NVS параметры.
 * Оптимизирована по убыванию размера полей (64-bit -> 32-bit -> float ->
 * strings -> bool) Размер структуры уменьшен за счет исключения неявного
 * padding'а.
 */
typedef struct {
    // --- ИДЕАЛЬНО ВЫРОВНЕННЫЙ ЗАГОЛОВОК (Ровно 4 байта) ---
    uint16_t version;   /**< 2 байт: Версия структуры. */
    uint16_t mqtt_port; /**< 2 байта: Порт MQTT брокера. */

    // 1. 32-битные целые числа и float (адреса кратны 4 байтам)
    float t_setting;      /**< Целевая температура теплоносителя, °C. */
    float t_hysteresis;   /**< Гистерезис термостата, °C. */
    int32_t reg_mode;     /**< Текущий режим работы (enum_reg_mode). */
    int32_t _reserved[4]; /**< 4x4 байтовых резервированных поля (16 байт) */

    // 2. Строковые массивы
    char mqtt_host[64];
    char mqtt_topic[64];
    char mqtt_client_id[32];
    char mqtt_user[32];
    char mqtt_pass[32];
    char admin_pass[32];

    // 3. 8-битные логические флаги (Каждый занимает ровно 1 байт)
    bool wifi_enabled;
    bool is_config_init;
    bool mqtt_enabled;
    bool rf433_enable;     /**< Флаг rf433 */
    bool first_boot_done;  /**< Флаг первого запуска */
    bool overshoot_enable; /**< Разрешать коррекцию перелета нагрева */

    // Блок из 6 флагов (6 байт) дополняем до границы 4 байт
    bool _reserv_bool[2];

} app_settings_t;

/**
 * @brief Динамический статус системы в ОЗУ.
 * Выровнен по убыванию (float [4 байта] -> bool [1 байт x 3]).
 */
typedef struct {
    float target_temp; /**< Текущая целевая температура (с учетом режимов). */
    bool relay;        /**< Текущее физическое состояние реле (true = включено). */
    bool thermostat;   /**< Состояние внешнего термостата (true = замкнут). */
    bool wifi;         /**< Статус подключения Wi-Fi. */
    bool _reserved;
} app_status_t;

/**
 * @brief Перечисление ВСЕХ параметров системы в строгом соответствии со
 * структурами.
 */
typedef enum {
    // === Группа 1: Настройки (NVS, защищены s_mutex_config) ===
    PARAM_CONFIG_VERSION,
    PARAM_SET_TARGET_TEMP,
    PARAM_SET_HYSTERESIS,
    PARAM_SET_REG_MODE,
    PARAM_SET_MQTT_PORT,

    PARAM_SET_MQTT_HOST,
    PARAM_SET_MQTT_TOPIC,
    PARAM_SET_MQTT_CLIENT_ID,
    PARAM_SET_MQTT_USER,
    PARAM_SET_MQTT_PASS,
    PARAM_SET_ADMIN_PASS,

    PARAM_SET_WIFI_ENABLED,
    PARAM_SET_CONFIG_INIT,
    PARAM_SET_MQTT_ENABLED,
    PARAM_SET_RF433_ENABLE,
    PARAM_SET_FIRST_BOOT_DONE,
    PARAM_SET_OVERSHOOT,
    // === Группа 2: Динамический статус (RAM, защищены s_mutex_status) ===
    PARAM_STAT_TARGET_TEMP,
    PARAM_STAT_RELAY,
    PARAM_STAT_THERMOSTAT,
    PARAM_STAT_WIFI,
    // ====== счетчик всегда последний!!
    APP_CFG_PARAM_COUNT
} app_cfg_param_t;

// app_param_update_t позволит передавать в функцию список из любого
// количества параметров и указателей на их новые значения:
typedef struct {
    app_cfg_param_t param; // Какую переменную меняем
    const void *p_val;     // Указатель на новое значение
} app_param_update_t;

/** Макрос инициализации app_status_t значениями по умолчанию. */
#define APP_STATUS_DEFAULT_LOAD()                                                                  \
    {.target_temp = 20.0f, .relay = false, .thermostat = false, .wifi = true, ._reserved = 0}

/**
 * @brief Заводские настройки приложения app_settings_t.
 */
#define APP_SETTINGS_DEFAULT()                                                                     \
    {.version = BOILER_CONFIG_VERSION,                                                             \
     .reg_mode = 0, /* MODE_EMERGENCY */                                                           \
     .mqtt_port = 1883,                                                                            \
     .t_setting = 20.0f,                                                                           \
     .t_hysteresis = 2.0f,                                                                         \
     .mqtt_host = "mqtt.local",                                                                    \
     .mqtt_topic = "boiler",                                                                       \
     .mqtt_client_id = "",                                                                         \
     .mqtt_user = "",                                                                              \
     .mqtt_pass = "",                                                                              \
     .admin_pass = "admin",                                                                        \
     .wifi_enabled = true,                                                                         \
     .is_config_init = false,                                                                      \
     .mqtt_enabled = false,                                                                        \
     .rf433_enable = true,                                                                         \
     .overshoot_enable = true,                                                                     \
     .first_boot_done = false,                                                                     \
     ._reserved = {0},                                                                             \
     ._reserv_bool = {0}}

/**
 * @brief Пакетная (транзакционная) запись нескольких параметров за один раз.
 * @param[in] updates Массив структур с параметрами и их новыми значениями.
 * @param[in] count Количество элементов в массиве обновлений.
 * @return true в случае успеха, false при ошибке или тайм-ауте мьютекса.
 *
 */
bool app_state_set_batch(const app_param_update_t *updates, size_t count);

/**
 * @brief Универсальное чтение любого параметра (из NVS или RAM).
 * @param[out] out_val Указатель на переменную для записи результата.
 * @param[in] param Идентификатор запрашиваемого параметра.
 * @return true в случае успеха, false при ошибке.
 */
bool app_state_get(void *out_val, app_cfg_param_t param);

/**
 * @brief Универсальная запись любого параметра (с автосохранением в NVS, если
 * нужно).
 * @param[in] in_val Указатель на новые данные для записи.
 * @return true в случае успеха, false при ошибке.
 */
bool app_state_set(app_cfg_param_t param, const void *in_val);

/**
 * @brief Инициализация модуля конфигурации.
 *
 * Создаёт мьютекс, загружает настройки и погодную таблицу из NVS.
 * @return ESP_OK при успешной инициализации, ESP_ERR_NO_MEM при нехватке
 * памяти.
 */
esp_err_t app_config_init(void);

/**
 * @brief Потокобезопасное чтение настроек.
 * @param[out] out_settings Указатель на структуру для копирования настроек. Не
 * должен быть NULL.
 */
void app_config_get(app_settings_t *out_settings);

/**
 * @brief Потокобезопасная запись настроек с сохранением в NVS.
 *
 * Копирует new_settings в s_settings, вызывает внутреннюю функцию сохранения.
 * @param[in] new_settings Указатель на новые настройки. Не должен быть NULL.
 */
void app_config_set(const app_settings_t *new_settings);

void app_status_get(app_status_t *out_status);

void app_status_set(const app_status_t *in_status);

// Быстрая блокировка
void app_settings_reference_lock(void);

// Геттер просто отдает прямой адрес скрытого массива только для чтения
const app_settings_t *app_settings_reference_get(void);

// Быстрая разблокировка
void app_settings_reference_unlock(void);

// Быстрая блокировка
void app_status_reference_lock(void);
// Геттер просто отдает прямой адрес скрытого массива только для чтения
const app_status_t *app_status_reference_get(void);
// Быстрая разблокировка
void app_status_reference_unlock(void);

#define WITH_STATUS_LOCK()                                                                         \
    for (bool _lock = (app_status_reference_lock(), true); _lock;                                  \
         _lock = (app_status_reference_unlock(), false))

#define WITH_CONFIG_LOCK()                                                                         \
    for (bool _lock = (app_settings_reference_lock(), true); _lock;                                \
         _lock = (app_settings_reference_unlock(), false))

#endif // APP_CONFIG_H
