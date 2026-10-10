#ifndef UI_MENU_H
#define UI_MENU_H

#include "app_config.h"
#include "esp_err.h"

/**
 * @brief Инициализация периферии (LCD, Encoder) и запуск UI задачи
 * @note Вызывать один раз в app_main() после инициализации NVS
 */
void init_menu_ui(void);
esp_err_t init_i2c_lcd(void);

#endif /* UI_MENU_H */
