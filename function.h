/**
 * @file function.h
 * @brief Общие вспомогательные функции и утилиты.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

// роверки попадания числа в диапазон от -50 до +60
// Макрос возвращает 1 (true) или 0 (false)
// #define IN_RANGE(val, min, max) ((val) >= (min) && (val) <= (max))

// // inline заменяет вызов функции прямой вставкой кода при компиляции
// static inline bool is_temp_valid(double temp) {
//     return temp >= -50.0 && temp <= 60.0;
// }

/**
 * @brief Получить текущее время в секундах.
 *
 * uint32_t намеренно используется для хранения времени.
 * Разность двух uint32_t корректно работает через переполнение
 * счётчика при условии, что измеряемый интервал существенно меньше
 * полного диапазона uint32_t.
 */
uint32_t app_get_time_sec(void);

/**
 * @brief Возвращает время в миллисекундах с момента старта.
 * @note Никогда не переполнится (запас 292 тысячи лет).
 */
uint64_t app_get_time_ms64(void);

/**
 * @brief Возвращает время в миллисекундах, усеченное до 32 бит.
 * @note Переполняется каждые 49.7 дней, но безопасно для подсчета интервалов.
 */
uint32_t app_get_time_ms32(void);

/**
 * @brief Возвращает строковое значение режима работы.
 *
 * @param mode
 */
const char *reg_mode_to_string(int32_t mode);

/**
 * @brief Генерирует имя для девайса.
 *
 * @param void
 */

const char *get_board_name(void);

/**
 * @brief Макрос безопасного освобождения указателя.
 *
 * Вызывает free() и обнуляет указатель.
 * @param p Указатель на освобождаемую память.
 */
#define FREE(p)                                                                                    \
    do {                                                                                           \
        free(p);                                                                                   \
        (p) = NULL;                                                                                \
    } while (0)

/**
 * @brief Обёртка над динамическим буфером с метаданными.
 */
typedef struct {
    char *data;  /**< Указатель на данные */
    size_t size; /**< Выделенный размер в байтах */
} safe_buffer_t;

/**
 * @brief Вычисляет длину строки, которая была бы сформирована через snprintf.
 *
 * Используется для предварительного определения размера буфера:
 *   char *buf = malloc(strsize(fmt, arg) + 1);
 *   snprintf(buf, ..., fmt, arg);
 */
#ifndef strsize
#define strsize(args...) snprintf(NULL, 0, args) + sizeof('\0')
#endif

/**
 * @brief Создаёт структуру safe_buffer_t с выделенным буфером.
 *
 * @param size Размер буфера в байтах.
 * @return Указатель на safe_buffer_t или NULL при ошибке.
 */
safe_buffer_t *create_buffer(size_t size);

/**
 * @brief Освобождает память, занятую safe_buffer_t и его данными.
 *
 * @param buffer Указатель на буфер для освобождения.
 */
void free_buffer(safe_buffer_t *buffer);

/**
 * @brief Изменяет размер внутреннего буфера.
 *
 * @param buffer Указатель на структуру буфера.
 * @param size Новый размер в байтах.
 * @return Новый размер при успехе, 0 при ошибке.
 */
size_t realloc_buffer(safe_buffer_t *buffer, size_t size);

/**
 * @brief Медианный фильтр по последним трём вызовам.
 *
 * @param value Новое измеренное значение.
 * @return Медианное значение из трёх последних.
 */
float median(float value);

/**
 * @brief Безопасное сравнение двух float с учётом погрешности.
 *
 * @param a Первое значение.
 * @param b Второе значение.
 * @param epsilon Допустимая абсолютная погрешность.
 * @return true если |a - b| <= epsilon.
 */
bool are_equal_float(float a, float b, float epsilon);

/**
 * @brief Формирует строку аптайма устройства (чч:мм:сс или чч:мм).
 *
 * format_uptime_string char lines_buf[4][32].
 * Формат зависит от длительности:
 * - < 100 ч — "HH:MM:SS"
 * - 100..999 ч — "HH:MM" (секунды отбрасываются)
 * - >= 1000 ч — "HH" (только часы)
 *
 * @param char *out_val_buf Указатель на буфер для записи результата lines_buf[32].
 */
void format_uptime_string(char *out_val_buf, size_t buf_size, uint32_t total_sec);

/**
 * @brief Конвертирует RSSI (дБм) в качество связи (0-100%)
 *
 * Диапазон расчета:
 * <= -90 дБм  --> 0% (связь невозможна или крайне нестабильна)
 * >= -30 дБм  --> 100% (отличный уровень)
 */
uint8_t get_wifi_quality_percentage(void);

void wifi_connected_info(char *my_ssid, size_t sz, int8_t *rssi);

const char *get_ssid_string();

#ifdef __cplusplus
}
#endif
