#include "ui_menu_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <encoder.h>

static const char *TAG = "UI_TASK";

/* ======================== Encoder callback ======================== */

void encoder_event_callback(const rotary_encoder_event_t *event, void *ctx) {
    input_event_t menu_event = EVENT_NONE;
    last_user_action_time = esp_timer_get_time() / 1000;
    if (event->type == RE_ET_CHANGED)
        menu_event = (event->diff > 0) ? EVENT_ROTATE_RIGHT : EVENT_ROTATE_LEFT;
    else if (event->type == RE_ET_BTN_CLICKED)
        menu_event = EVENT_SHORT_PRESS;
    else if (event->type == RE_ET_BTN_LONG_PRESSED)
        menu_event = EVENT_LONG_PRESS;
    if (menu_event != EVENT_NONE)
        xQueueSendToBack(input_queue, &menu_event, 0);
}

/* ======================== ui_task ======================== */

void ui_task(void *pvParameters) {
    input_event_t event;
    menu_state_t local_state = (menu_state_t)-1;
    int32_t local_l1_idx = 0;
    int32_t local_l2_idx = 0;
    int32_t local_offset = 0;
    int32_t backup_val = 0;
    char lines_buf[4][32];

    lcd_clear();

    while (1) {
        bool in_menu =
            (local_state == STATE_MENU_L1 || local_state == STATE_MENU_L2 ||
             local_state == STATE_EDIT_VALUE || local_state == STATE_ABOUT_SCREEN);

        TickType_t wait_ticks = in_menu ? pdMS_TO_TICKS(1000) : pdMS_TO_TICKS(500);

        if (xQueueReceive(input_queue, &event, wait_ticks) == pdTRUE) {
            if (!in_menu) {
                /* === РЕЖИМ КАРУСЕЛИ (не меню) === */
                if (event == EVENT_LONG_PRESS) {
                    ESP_LOGI(TAG, "Entering Menu Mode.");
                    last_user_action_time = esp_timer_get_time() / 1000;
                    local_state = STATE_MENU_L1;
                    local_l1_idx = 0;

                    snprintf(lines_buf[0], sizeof(lines_buf[0]), "> %s", STR_L1_0);
                    snprintf(lines_buf[1], sizeof(lines_buf[1]), "  %s", STR_L1_1);

                    lcd_refresh_smart_2004(lines_buf[0], lines_buf[1], " ", " ", true);
                    continue;
                } else if (event == EVENT_ROTATE_RIGHT || event == EVENT_ROTATE_LEFT) {
                    carousel_manual_mode = true;
                    carousel_click_counter++;

                    if (carousel_click_counter >= CAROUSEL_DEBOUNCE_CLICKS) {
                        if (event == EVENT_ROTATE_RIGHT) {
                            carousel_manual_page =
                                (carousel_manual_page + 1) % SCR_PAGE_SZ;
                        } else {
                            carousel_manual_page =
                                (carousel_manual_page + SCR_PAGE_SZ - 1) % SCR_PAGE_SZ;
                        }
                        carousel_click_counter = 0;
                        last_user_action_time = esp_timer_get_time() / 1000;
                        ESP_LOGD(TAG,
                                 "Carousel manual switch to page %d (after %d "
                                 "clicks)",
                                 carousel_manual_page, CAROUSEL_DEBOUNCE_CLICKS);
                    } else {
                        ESP_LOGD(TAG, "Carousel click %d/%d", carousel_click_counter,
                                 CAROUSEL_DEBOUNCE_CLICKS);
                    }
                    continue;
                } else {
                    continue;
                }
            }

            /* --- ЛОГИКА ПЕРЕКЛЮЧЕНИЯ СОСТОЯНИЙ --- */
            switch (local_state) {

            case STATE_MENU_L1:

                if (event == EVENT_ROTATE_RIGHT && local_l1_idx < 1)
                    local_l1_idx++;
                else if (event == EVENT_ROTATE_LEFT && local_l1_idx > 0)
                    local_l1_idx--;
                else if (event == EVENT_SHORT_PRESS) {
                    if (local_l1_idx == 0) {
                        local_state = STATE_MENU_L2;
                        local_l2_idx = 0;
                        local_offset = 0;
                    } else if (local_l1_idx == 1) {
                        local_state = STATE_ABOUT_SCREEN;
                    }
                } else if (event == EVENT_LONG_PRESS) {
                    force_carousel_reset = true;
                    local_state = (menu_state_t)-1;
                    continue;
                }
                break;

            case STATE_MENU_L2:

                if (event == EVENT_ROTATE_RIGHT && local_l2_idx < UI_PARAM_COUNT - 1) {
                    local_l2_idx++;
                    if (local_l2_idx - local_offset > 3)
                        local_offset++;
                } else if (event == EVENT_ROTATE_LEFT && local_l2_idx > 0) {
                    local_l2_idx--;
                    if (local_l2_idx < local_offset)
                        local_offset--;
                } else if (event == EVENT_SHORT_PRESS) {
                    setup_param_t *item = &s_param_list[local_l2_idx];
                    backup_val = *(item->val_ptr);
                    local_state = STATE_EDIT_VALUE;
                } else if (event == EVENT_LONG_PRESS) {
                    lcd_refresh_smart_2004(" ", " ", " ", " ", true);
                    local_state = STATE_MENU_L1;
                }
                break;

            case STATE_EDIT_VALUE: {
                setup_param_t *item = &s_param_list[local_l2_idx];

                if (event == EVENT_ROTATE_RIGHT && *(item->val_ptr) < item->max_val) {
                    (*(item->val_ptr))++;
                } else if (event == EVENT_ROTATE_LEFT &&
                           *(item->val_ptr) > item->min_val) {
                    (*(item->val_ptr))--;

                } else if (event == EVENT_SHORT_PRESS) {

                    ESP_LOGI(TAG, "Edit canceled. Restoring original value.");
                    *(item->val_ptr) = backup_val;
                    local_state = STATE_MENU_L2;

                } else if (event == EVENT_LONG_PRESS) {
                    ESP_LOGI(TAG, "Edit confirmed! Saving to NVS Flash...");

                    if (item->callback != NULL) {
                        ESP_LOGI(TAG, "Executing item callback...");
                        item->callback(item);
                    }

                    /* НЕ ЗАБЫВАЕМ ДОБАВЛЯТЬ СЮДА РЕДАКТИРУЕМЫЕ ПАРАМЕТРЫ!!!! */
                    /* ВТОРОЙ ТАКОЙ БЛОК  АВТОВЫХОД ПО ТАЙМАУТУ!!! */
                    ui_settings_save_all();
                    vTaskDelay(pdMS_TO_TICKS(40));
                    local_state = STATE_MENU_L2;
                }
                break;
            }

            case STATE_ABOUT_SCREEN:
                if (event == EVENT_SHORT_PRESS || event == EVENT_LONG_PRESS)
                    local_state = STATE_MENU_L1;
                break;

            default:
                break;
            }

            /* --- ОТРИСОВКА ПОСЛЕ ОБРАБОТКИ СОБЫТИЯ --- */
            if (local_state != (menu_state_t)-1) {
                if (local_state == STATE_MENU_L1) {
                    snprintf(lines_buf[0], sizeof(lines_buf[0]), "%s%s",
                             (local_l1_idx == 0) ? "> " : "  ", STR_L1_0);
                    snprintf(lines_buf[1], sizeof(lines_buf[1]), "%s%s",
                             (local_l1_idx == 1) ? "> " : "  ", STR_L1_1);
                    lcd_refresh_smart_2004(lines_buf[0], lines_buf[1], " ", " ", true);
                } else if (local_state == STATE_MENU_L2) {
                    for (int i = 0; i < 4; i++) {
                        int32_t item_idx = local_offset + i;

                        if (item_idx < UI_PARAM_COUNT) {
                            setup_param_t item = s_param_list[item_idx];
                            char val_buf[12] = {0};

                            if (item.display_type == PARAM_TYPE_INT) {
                                snprintf(val_buf, sizeof(val_buf), "%" PRId32,
                                         *(item.val_ptr));
                            } else if (item.display_type == PARAM_TYPE_ENUM &&
                                       item.enum_to_str != NULL) {
                                snprintf(val_buf, sizeof(val_buf), "%s",
                                         item.enum_to_str(*(item.val_ptr)));
                            } else {
                                snprintf(val_buf, sizeof(val_buf), "%s",
                                         *(item.val_ptr) ? "ON" : "OFF");
                            }

                            snprintf(lines_buf[i], sizeof(lines_buf[i]),
                                     "%c%-13.13s%6.6s",
                                     (local_l2_idx == item_idx) ? '>' : ' ', item.name,
                                     val_buf);
                        } else {
                            snprintf(lines_buf[i], sizeof(lines_buf[i]), " ");
                        }
                    }
                    lcd_refresh_smart_2004(lines_buf[0], lines_buf[1], lines_buf[2],
                                           lines_buf[3], false);
                } else if (local_state == STATE_EDIT_VALUE) {
                    setup_param_t item = s_param_list[local_l2_idx];

                    snprintf(lines_buf[0], sizeof(lines_buf[0]),
                             (*(item.val_ptr) != backup_val) ? STR_EDIT_UNSAVED
                                                             : STR_EDIT_PREFIX,
                             item.name);

                    if (item.display_type == PARAM_TYPE_INT) {
                        snprintf(lines_buf[1], sizeof(lines_buf[1]), STR_VAL_INT_FMT,
                                 *(item.val_ptr));
                    } else if (item.display_type == PARAM_TYPE_ENUM &&
                               item.enum_to_str != NULL) {
                        snprintf(lines_buf[1], sizeof(lines_buf[1]), "Val: < %s >",
                                 item.enum_to_str(*(item.val_ptr)));
                    } else {
                        snprintf(lines_buf[1], sizeof(lines_buf[1]), "%s",
                                 *(item.val_ptr) ? STR_VAL_BOOL_ON : STR_VAL_BOOL_OFF);
                    }

                    snprintf(lines_buf[2], sizeof(lines_buf[2]), "[Click] -> Cancel");
                    snprintf(lines_buf[3], sizeof(lines_buf[3]), "[Hold]  -> Save");

                    lcd_refresh_smart_2004(lines_buf[0], lines_buf[1], lines_buf[2],
                                           lines_buf[3], false);
                } else if (local_state == STATE_ABOUT_SCREEN) {
                    about_screen(lines_buf);
                    lcd_refresh_smart_2004(lines_buf[0], lines_buf[1], lines_buf[2],
                                           lines_buf[3], true);
                }
            }
        }

        /* --- СТАРТОВЫЙ ЭКРАН (карусель) --- */
        if (local_state == (menu_state_t)-1) {
            ui_sync_from_config();

            uint32_t now = esp_timer_get_time() / 1000;
            bool is_auto_carousel =
                !carousel_manual_mode || ((now - last_user_action_time) >= 30000);
            if (carousel_manual_mode && is_auto_carousel) {
                carousel_manual_mode = false;
                carousel_manual_page = 0;
                ESP_LOGI(TAG, "Returning to auto-carousel after 30s timeout");
            }
            lcd_start_screen(lines_buf, is_auto_carousel);
        }

        /* --- АВТО-ВЫХОД ПО ТАЙМАУТУ --- */
        if (in_menu) {
            uint32_t now = esp_timer_get_time() / 1000;
            if ((now - last_user_action_time) >= MENU_TIMEOUT_MS) {
                ESP_LOGW(TAG, "Menu timeout! Auto-closing.");
                if (local_state == STATE_EDIT_VALUE) {
                    /* НЕ ЗАБЫВАЕМ ДОБАВЛЯТЬ СЮДА РЕДАКТИРУЕМЫЕ ПАРАМЕТРЫ!!!! */
                    /* ВТОРОЙ ТАКОЙ БЛОК  case STATE_EDIT_VALUE: */
                    ui_settings_save_all();
                }
                local_state = (menu_state_t)-1;
                force_carousel_reset = true;
                continue;
            } else {
                if (local_state != STATE_EDIT_VALUE) {
                    ui_sync_from_config();
                }
            }
        }
    }
}
