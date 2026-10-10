#include "app_config.h"
#include "ui_menu_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// #include "esp_log.h"
#include "esp_timer.h"
// #include "freertos/FreeRTOS.h"
#include <time.h>

#include "app_radio.h"
#include "app_wifi.h"
#include "function.h"
#include "room_climate.h"
#include "ui_driver.h"
#include <math.h>

//
// Вспомогательная структура для выгрузки температур
typedef struct {
    float t_tube;
    float t_outside;
    float t_room;
    bool is_tube_error;
    bool is_outside_error;
    bool is_room_error;
    bool _ok;
} ui_local_temps_t;

static void ui_format_datetime_2004(char *line_time, char *line_date) {
    if (line_time == NULL || line_date == NULL)
        return;

    time_t now;
    struct tm timeinfo;

    // Получаем текущее системное время
    time(&now);
    localtime_r(&now, &timeinfo);

    // Буферы для промежуточного форматирования
    char raw_time[12]; // Для "HH:MM:SS" (8 симв + '\0')
    char raw_date[18]; // Для "Mon, 21 Sep 2026" (~16 симв + '\0')

    // Форматируем время: %H - часы (24ч), %M - минуты, %S - секунды
    strftime(raw_time, sizeof(raw_time), "%H:%M:%S", &timeinfo);

    // Форматируем дату: %a - сокр. день недели, %d - число, %b - сокр. месяц,
    // %Y - год
    strftime(raw_date, sizeof(raw_date), "%a, %d %b %Y", &timeinfo);

    // Центрируем результаты ровно под ширину дисплея 2004
    // Магия "%*s": вычисляем сколько пробелов нужно добавить для центрирования
    int time_len = strlen(raw_time);
    int time_padding = (20 - time_len) / 2;
    snprintf(line_time, 21, "%*s%s%-*s", time_padding, "", raw_time,
             20 - time_len - time_padding, "");

    int date_len = strlen(raw_date);
    int date_padding = (20 - date_len) / 2;
    snprintf(line_date, 21, "%*s%s%-*s", date_padding, "", raw_date,
             20 - date_len - date_padding, "");
}

/* ======================== lcd_clear ======================== */

void lcd_clear(void) {
    lcd_send_cmd(0x01);
}

/* ======================== Форматирование ======================== */

void ui_format_split_line(char *out_buf, const char *text_left, const void *value,
                          ui_val_type_t type, const char *unit_right) {
    if (out_buf == NULL || value == NULL)
        return;

    // Защита от NULL в указателе строки юнитов
    if (unit_right == NULL) {
        unit_right = "";
    }

    char right_part_buf[21] = {0};
    const int max_screen_width = 20;

    /* ЭТАП 1: Распознаём тип данных */
    if (type == VAL_TYPE_FLOAT) {
        float float_val = *(const float *)value;
        snprintf(right_part_buf, sizeof(right_part_buf), "%.1f%s", float_val, unit_right);
    } else if (type == VAL_TYPE_INT) {
        int32_t int_val = *(const int32_t *)value;
        snprintf(right_part_buf, sizeof(right_part_buf), "%" PRId32 "%s", int_val,
                 unit_right);
    } else if (type == VAL_TYPE_INT8) {
        // Явно приводим к int8_t, печатаем через стандартный d
        int8_t int8_val = *(const int8_t *)value;
        snprintf(right_part_buf, sizeof(right_part_buf), "%d%s", int8_val, unit_right);
    } else if (type == VAL_TYPE_STRING) {
        const char *str_val = (const char *)value;
        int max_chars_to_read = sizeof(right_part_buf) - 1 - strlen(unit_right);
        if (max_chars_to_read < 0)
            max_chars_to_read = 0;

        snprintf(right_part_buf, sizeof(right_part_buf), "%.*s%s", max_chars_to_read,
                 str_val, unit_right);
    }

    /* ЭТАП 2: Собираем финальную строку */
    int right_len = (int)strlen(right_part_buf);
    int left_width = max_screen_width - right_len;
    if (left_width < 0)
        left_width = 0;

    // Заменяем %s на %.*s и передаем right_len.
    // Теперь компилятор видит, что правая часть жестко контролируется по длине.
    snprintf(out_buf, max_screen_width + 1, "%-*.*s%.*s", left_width, left_width,
             text_left ? text_left : "", right_len, right_part_buf);
}

/* ======================== lcd_refresh_smart_2004 ======================== */

void lcd_refresh_smart_2004(const char *line0, const char *line1, const char *line2,
                            const char *line3, bool force_clear) {
    static char screen_cache[4][20] = {
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
         ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
         ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
         ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
         ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    };

    if (force_clear) {
        lcd_clear();
        memset(screen_cache, ' ', sizeof(screen_cache));
    }

    // Безопасная подмена NULL на пустые строки для предотвращения краша
    // snprintf
    char input_buffers[4][21];
    snprintf(input_buffers[0], 21, "%-20.20s", line0 ? line0 : "");
    snprintf(input_buffers[1], 21, "%-20.20s", line1 ? line1 : "");
    snprintf(input_buffers[2], 21, "%-20.20s", line2 ? line2 : "");
    snprintf(input_buffers[3], 21, "%-20.20s", line3 ? line3 : "");

    for (uint8_t row = 0; row < 4; row++) {
        bool need_cursor_positioning =
            true; // Флаг: нужно ли принудительно выставить курсор

        for (uint8_t col = 0; col < 20; col++) {
            char new_char = input_buffers[row][col];

            if (new_char != screen_cache[row][col]) {
                // Если мы до этого пропускали символы или это начало строки,
                // принудительно отправляем команду установки курсора
                if (need_cursor_positioning) {
                    lcd_set_cursor(col, row);
                    need_cursor_positioning = false; // Курсор на месте, следующие символы
                                                     // пойдут автосдвигом
                }

                lcd_send_data((uint8_t)new_char);
                screen_cache[row][col] = new_char;
            } else {
                // Символы совпали! Мы пропускаем отправку.
                // Так как мы пропустили позицию, автосдвиг дисплея нам больше
                // не помогает. Для следующего изменившегося символа мы ОБЯЗАНЫ
                // будем вызвать lcd_set_cursor.
                need_cursor_positioning = true;
            }
        }
    }
}

/* ======================== lcd_start_screen ======================== */

const char *const UI_PAGES_STR[] = {
    "INDOOR :",     "OUTDOOR :", "HUMIDITY : ", "PRESSURE : ", "TUBE : ",
    "TARGET [t]: ", "UPTIME : ", "MODE : ",     "PUMP : ",     "SSID :",
    "LEVEL",        "WAIT...",   "DISABLE",     "RSSI : ",     "EXTERNAL :",
};
const char *const STR_UNIT_CELSIUS = "\xDF"
                                     "C";

void lcd_start_screen(char lines_buf[4][32], bool is_auto_carousel) {

    static screen_page_t current_page = SCR_PAGE_0;
    static uint32_t last_page_switch_time = 0;

    uint32_t now = esp_timer_get_time() / 1000;

    if (force_carousel_reset) {
        current_page = SCR_PAGE_0;
        last_page_switch_time = now;
        force_carousel_reset = false;
        carousel_manual_mode = false;
        lcd_refresh_smart_2004(" ", " ", " ", " ", true);
    }

    if (is_auto_carousel && !carousel_manual_mode) {
        if ((now - last_page_switch_time) >= 10000) {
            current_page++;
            if (current_page >= 2) {
                current_page = SCR_PAGE_0;
            }
            last_page_switch_time = now;
            lcd_refresh_smart_2004(" ", " ", " ", " ", true);
        }
    }

    screen_page_t display_page =
        is_auto_carousel ? current_page : (screen_page_t)carousel_manual_page;

    switch (display_page) {

    case SCR_PAGE_0:
    case SCR_PAGE_1: {
        static ui_local_temps_t t_page;

        // Быстрая блокировка общего котла
        WITH_TEMPERATURE_LOCK() {
            // Геттер просто отдает прямой адрес скрытого массива
            const TemperatureStorage_t *s_temp = temperature_get_reference();
            t_page.t_tube = s_temp->channels[CH_INDEX_PIPE].temperature;
            t_page.is_tube_error = s_temp->channels[CH_INDEX_PIPE].is_error;
            t_page.t_outside = s_temp->channels[CH_INDEX_OUTDOOR].temperature;
            t_page.is_outside_error = s_temp->channels[CH_INDEX_OUTDOOR].is_error;
            t_page.t_room = s_temp->channels[CH_INDEX_ROOM].temperature;
            // Быстрая разблокировка
        }

        // собираем строки начиная с первой
        ui_format_split_line(lines_buf[0], UI_PAGES_STR[0], &t_page.t_room,
                             VAL_TYPE_FLOAT, STR_UNIT_CELSIUS);

        if (display_page == SCR_PAGE_0) {
            // Смотрим, по какому датчику принимать показания уличной температуры.
            // приоритет проводному, но если он в ошибке, то пробуем выносной
            if (t_page.is_outside_error ||
                IS_TEMP_OUTSIDE_OUT_OF_RANGE(t_page.t_outside)) {
                float t_temp = NAN;

                WITH_RADIO433_LOCK() {
                    const radio433_data_t *rdt = radio433_get_reference();
                    if (rdt)
                        t_temp = rdt->temperature;
                }

                if (isnan(t_temp)) {
                    ui_format_split_line(lines_buf[1], UI_PAGES_STR[1], "ERROR",
                                         VAL_TYPE_STRING, "");
                } else {
                    ui_format_split_line(lines_buf[1], UI_PAGES_STR[14], &t_temp,
                                         VAL_TYPE_FLOAT, STR_UNIT_CELSIUS);
                }

            } else {
                ui_format_split_line(lines_buf[1], UI_PAGES_STR[1], &t_page.t_outside,
                                     VAL_TYPE_FLOAT, STR_UNIT_CELSIUS);
            }

            WITH_CLIMAT_LOCK() {
                const ClimateStorage_t *climat = app_climate_storage_get();
                if (climat->ch0_ok) {
                    int val = (int)(climat->ch0_h + 0.5f);
                    ui_format_split_line(lines_buf[2], UI_PAGES_STR[2], &val,
                                         VAL_TYPE_INT, " %");
                }
                if (climat->ch1_ok) {
                    int mmrst = (int)((climat->ch1_p * 0.00750062f) + 0.5f);
                    ui_format_split_line(lines_buf[3], UI_PAGES_STR[3], &mmrst,
                                         VAL_TYPE_INT, " mm.");
                }
            }

        } else {

            static uint8_t heat_anim_frame = 0;
            static char left_text_buf[16]; // Буфер на 16 символов
            static bool is_heat;
            static int target;

            WITH_STATUS_LOCK() {
                const app_status_t *app = app_status_reference_get();
                is_heat = app->relay;
                target = (int)(app->target_temp + 0.5f);
            }

            if (is_heat) {
                // Выбираем стрелочки в зависимости от кадра
                const char *anim_str = "";
                switch (heat_anim_frame) {
                case 0:
                    anim_str = "";
                    break;
                case 1:
                    anim_str = " >";
                    break;
                case 2:
                    anim_str = " >>";
                    break;
                default:
                    anim_str = " >>>";
                    break; // Кадр 3
                }

                // Безопасно собираем строку: "Tube T:" + стрелочки
                snprintf(left_text_buf, sizeof(left_text_buf), "%s%s", UI_PAGES_STR[4],
                         anim_str);

                // Инкрементируем счетчик кадров (0, 1, 2, 3)
                heat_anim_frame = (heat_anim_frame + 1) % 4;
            } else {
                // Если нагрева нет, сбрасываем анимацию и пишем стандартный
                // текст
                heat_anim_frame = 0;
                snprintf(left_text_buf, sizeof(left_text_buf), "%s", UI_PAGES_STR[4]);
            }

            ui_format_split_line(lines_buf[1], UI_PAGES_STR[7],
                                 reg_mode_to_string(s_settings.reg_mode), VAL_TYPE_STRING,
                                 "");

            ui_format_split_line(lines_buf[2], left_text_buf, &t_page.t_tube,
                                 VAL_TYPE_FLOAT, STR_UNIT_CELSIUS);

            ui_format_split_line(lines_buf[3], UI_PAGES_STR[5], &target, VAL_TYPE_INT,
                                 STR_UNIT_CELSIUS);
        }

    }

    break;

    case SCR_PAGE_2: {

        ui_format_datetime_2004(lines_buf[0], lines_buf[1]);

        ui_format_split_line(lines_buf[2], UI_PAGES_STR[7],
                             reg_mode_to_string(s_settings.reg_mode), VAL_TYPE_STRING,
                             "");

        WITH_STATUS_LOCK() {
            const app_status_t *l_status = app_status_reference_get();
            ui_format_split_line(lines_buf[3], UI_PAGES_STR[8],
                                 l_status->relay ? "ON" : "OFF", VAL_TYPE_STRING, "");
        }

    }

    break;

    case SCR_PAGE_3: {

        if (wifi_is_connected()) {
            const char *ssid_buffer = get_ssid_string(false);
            int8_t rssi = get_wifi_quality_percentage();
            // wifi_connected_info(ssid_buffer, sizeof(ssid_buffer), &rssi);
            ui_format_split_line(lines_buf[0], UI_PAGES_STR[9], ssid_buffer,
                                 VAL_TYPE_STRING, "");
            ui_format_split_line(lines_buf[1], UI_PAGES_STR[10], &(rssi), VAL_TYPE_INT8,
                                 "%");
        } else {
            if (s_settings.wifi_enabled) {
                ui_format_split_line(lines_buf[0], STR_L2_0_4, UI_PAGES_STR[11],
                                     VAL_TYPE_STRING, "");
            } else {
                ui_format_split_line(lines_buf[1], STR_L2_0_4, UI_PAGES_STR[12],
                                     VAL_TYPE_STRING, "");
            }
        }
    }

    break;

    default:
        current_page = SCR_PAGE_0;
        break;
    }

    lcd_refresh_smart_2004(lines_buf[0], lines_buf[1], lines_buf[2], lines_buf[3], false);
}

/* ======================== about_screen ======================== */

void about_screen(char lines_buf[4][32]) {
    uint32_t uptime_sec = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    char formatted_time[10] = {0};
    format_uptime_string(formatted_time, sizeof(formatted_time), uptime_sec);
    snprintf(lines_buf[1], 32, "%-11.11s%9s", UI_PAGES_STR[6], formatted_time);
    snprintf(lines_buf[0], sizeof(lines_buf[0]), "%s", STR_L2_1_0);

    if (wifi_is_connected()) {
        char ssid_buffer[33] = {0};
        int8_t rssi;
        wifi_connected_info(ssid_buffer, sizeof(ssid_buffer), &rssi);
        ui_format_split_line(lines_buf[2], UI_PAGES_STR[9], &(ssid_buffer),
                             VAL_TYPE_STRING, "");
        ui_format_split_line(lines_buf[3], UI_PAGES_STR[13], &(rssi), VAL_TYPE_INT8,
                             "dBm");
    } else {
        if (s_settings.wifi_enabled) {
            snprintf(lines_buf[2], sizeof(lines_buf[2]), "%s %s", STR_L2_0_4,
                     UI_PAGES_STR[11]);
        } else {
            snprintf(lines_buf[2], sizeof(lines_buf[2]), "%s %s", STR_L2_0_4,
                     UI_PAGES_STR[12]);
        }
        snprintf(lines_buf[3], sizeof(lines_buf[3]), "%s", "");
    }
}
