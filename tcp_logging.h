#ifndef TCP_LOGGING_H
#define TCP_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief Инициализирует потокобезопасный Ring Buffer, перехватывает стандартный
 *        вывод ESP_LOGx и запускает фоновую FreeRTOS-задачу TCP-сервера.
 * 
 * @note Вызывать один раз в app_main(), до или сразу после инициализации Wi-Fi.
 */
void init_tcp_logging(void);

/**
 * @brief Динамически включает или выключает кэширование логов для Web.
 * @param enable true — выделить память под кэш, false — полностью освободить ОЗУ.
 * @return true в случае успеха, false если не хватило памяти.
 */
bool tcp_logging_set_web_enable(bool enable);

/**
 * @brief Проверяет, активно ли сейчас веб-логирование.
 */
bool tcp_logging_is_web_enabled(void);

/**
 * @brief Формирует JSON-массив из последних логов для HTML-интерфейса.
 * @param out_buf Буфер, куда записать JSON (выделяется на стороне HTTP-сервера).
 * @param max_len Максимальный размер буфера.
 * @return Количество записанных байт.
 * 
 * Пример вывода: ["Лог 1\n", "Лог 2\n", "Лог 3\n"]
 */
size_t tcp_logging_get_json(char *out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // TCP_LOGGING_H
