#ifndef UI_MENU_INTERNAL_H
#define UI_MENU_INTERNAL_H

#include "app_config.h"
#include "app_temperature.h"
#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <encoder.h>

/* ======================== Типы ======================== */

typedef enum {
    UI_PARAM_0 = 0,
    UI_PARAM_1,
    UI_PARAM_2,
    UI_PARAM_3,
    UI_PARAM_4,
    UI_PARAM_5,
    UI_PARAM_6,
    UI_PARAM_COUNT
} ui_param_id_t;

typedef enum { VAL_TYPE_INT, VAL_TYPE_FLOAT, VAL_TYPE_INT8, VAL_TYPE_STRING } ui_val_type_t;

typedef enum { SCR_PAGE_0 = 0, SCR_PAGE_1, SCR_PAGE_2, SCR_PAGE_3, SCR_PAGE_SZ } screen_page_t;

typedef enum {
    EVENT_NONE,
    EVENT_ROTATE_RIGHT,
    EVENT_ROTATE_LEFT,
    EVENT_SHORT_PRESS,
    EVENT_LONG_PRESS
} input_event_t;

typedef enum {
    STATE_MENU_L1,
    STATE_MENU_L2,
    STATE_EDIT_VALUE,
    STATE_ABOUT_SCREEN,
    STATE_CAROUSEL_MANUAL
} menu_state_t;

typedef enum { PARAM_TYPE_INT, PARAM_TYPE_BOOL, PARAM_TYPE_ENUM } param_display_type_t;

struct setup_param_t;
typedef void (*menu_callback_t)(struct setup_param_t *item);
typedef const char *(*enum_string_converter_t)(int32_t value);

typedef struct setup_param_t {
    const char *name;
    param_display_type_t display_type;
    int32_t *val_ptr;
    int32_t min_val;
    int32_t max_val;
    menu_callback_t callback;
    enum_string_converter_t enum_to_str;
} setup_param_t;

typedef struct menu_param_setings_t {
    int32_t t_setup;
    int32_t t_hysteresis;
    int32_t regulator_mode;
    int32_t wifi_enable;
    int32_t rf433_enable;
    int32_t reserv;
    int32_t reboot;
} menu_param_setings_t;

/* ======================== Макросы ======================== */

#define MENU_TIMEOUT_MS (20 * 1000)
#define CAROUSEL_DEBOUNCE_CLICKS 2

/* ======================== Внешние данные ======================== */

extern app_settings_t s_settings;

extern setup_param_t s_param_list[];
extern menu_param_setings_t s_menu_param;

extern QueueHandle_t input_queue;
extern rotary_encoder_handle_t re_handle;

extern volatile uint32_t last_act_ms;
extern volatile bool force_carousel_reset;
extern bool carousel_manual_mode;
extern int32_t carousel_manual_page;
extern int32_t carousel_click_counter;

/* ======================== Строковые константы ==================== */

extern const char *const STR_L1_0;
extern const char *const STR_L1_1;

extern const char *const STR_L2_0_0;
extern const char *const STR_L2_0_1;
extern const char *const STR_L2_0_2;
extern const char *const STR_L2_0_3;
extern const char *const STR_L2_0_4;
extern const char *const STR_L2_0_5;
extern const char *const STR_L2_0_6;

extern const char *const STR_L2_1_0;

extern const char *const STR_EDIT_PREFIX;
extern const char *const STR_EDIT_UNSAVED;

extern const char *const STR_VAL_INT_FMT;
extern const char *const STR_VAL_BOOL_ON;
extern const char *const STR_VAL_BOOL_OFF;

/* ======================== Прототипы ======================== */

/* Экраны */
void lcd_refresh_smart_2004(const char *line0, const char *line1, const char *line2,
                            const char *line3, bool force_clear);
void lcd_start_screen(char lines_buf[4][32], bool is_auto_carousel);
void about_screen(char lines_buf[4][32]);
void lcd_clear(void);

/* Форматирование */
void ui_format_split_line(char *out_buf, const char *text_left, const void *value,
                          ui_val_type_t type, const char *unit_right);
const char *reg_mode_to_string(int32_t mode);

/* Sync */
void ui_sync_from_config(void);
void ui_settings_save_all(void);

/* Encoder callback — определён в ui_menu_task.c */
extern void encoder_event_callback(const rotary_encoder_event_t *event, void *ctx);

/* ui_task — определён в ui_menu_task.c */
extern void ui_task(void *pvParameters);

#endif /* UI_MENU_INTERNAL_H */
