#include "esp_err.h"
#include "ui_menu_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <encoder.h>

#include "app_config.h"
#include "app_radio.h"
#include "app_temperature.h"
#include "app_wifi.h"
#include "ui_driver.h"

// Трюк для превращения числа в строку на этапе компиляции
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

/* ======================== Строки локализиции ==================== */

const char *const STR_L1_0 = "Settings";
const char *const STR_L1_1 = "About";

const char *const STR_L2_0_0 = "Target T";
const char *const STR_L2_0_1 = "Hysteresis";
const char *const STR_L2_0_2 = "Mode";
const char *const STR_L2_0_3 = "Weather Comp:";
const char *const STR_L2_0_4 = "Wi-Fi:";
const char *const STR_L2_0_5 = "RF433 Sensor:";
const char *const STR_L2_0_6 = "REBOOT:";

const char *const STR_L2_1_0 = "Boiler FW: " TOSTRING(BOILER_CONFIG_VERSION);

const char *const STR_EDIT_PREFIX = "Set: %s";
const char *const STR_EDIT_UNSAVED = "Set: %s *";

const char *const STR_VAL_INT_FMT = "Val: < %" PRId32 " >";
const char *const STR_VAL_BOOL_ON = "Val: < ON >";
const char *const STR_VAL_BOOL_OFF = "Val: < OFF >";

const char *const TAG = "UI";

/* ======================== Статические переменные ================== */

app_settings_t s_settings;
setup_param_t s_param_list[UI_PARAM_COUNT];
menu_param_setings_t s_menu_param;
QueueHandle_t input_queue = NULL;

rotary_encoder_handle_t re_handle = NULL;
volatile uint32_t last_act_ms = 0;
volatile bool force_carousel_reset = false;
bool carousel_manual_mode = false;
int32_t carousel_manual_page = 0;
int32_t carousel_click_counter = 0;

/* ======================== Forward declarations ==================== */

extern void encoder_event_callback(const rotary_encoder_event_t *event, void *ctx);

/* ======================== Sync / Callback ======================== */

void ui_sync_from_config(void) {

    app_config_get(&s_settings);
    s_menu_param.t_setup = (int32_t)s_settings.t_setting;
    s_menu_param.t_hysteresis = (int32_t)s_settings.t_hysteresis;
    s_menu_param.regulator_mode = (int32_t)s_settings.reg_mode;
    s_menu_param.wifi_enable = (int32_t)s_settings.wifi_enabled;
    s_menu_param.rf433_enable = (int32_t)s_settings.rf433_enable;
    ESP_LOGD(TAG, "UI variables synchronized.");
}

void ui_settings_save_all(void) {

    s_settings.t_setting = (float)s_menu_param.t_setup;
    s_settings.t_hysteresis = (float)s_menu_param.t_hysteresis;
    s_settings.reg_mode = (int32_t)s_menu_param.regulator_mode;
    s_settings.wifi_enabled = (bool)s_menu_param.wifi_enable;
    s_settings.rf433_enable = (bool)s_menu_param.rf433_enable;
    app_config_set(&s_settings);
    ESP_LOGI(TAG, "UI variables synchronized with AppConfig.");
}

void lcd_reset_home_carousel(void) {
    force_carousel_reset = true;
}

static void wifi_action_callback(setup_param_t *item) {
    ESP_LOGI(TAG, "Executing %s callback. Value: %i", item->name, *item->val_ptr);
    bool val = (bool)*item->val_ptr;
    if (val) {
        app_wifi_enable();
    } else {
        app_wifi_disable();
    }
}

static void menu_reboot_callback(setup_param_t *item) {
    ESP_LOGW(TAG, "Executing %s callback. Value: %i", item->name, *item->val_ptr);
    bool val = (bool)*item->val_ptr;
    if (val) {
        lcd_reset_home_carousel();
        app_wifi_session_disable();
        // Даем прокашляться секунду
        vTaskDelay(pdMS_TO_TICKS(1000));
        // Перезагрузка устройства
        esp_restart();
    }
}

static void menu_rf433_callback(setup_param_t *item) {
    ESP_LOGI(TAG, "Executing %s callback. Value: %i", item->name, *item->val_ptr);
    bool val = (bool)*item->val_ptr;
    app_rf433_switch_enable(val);
}

/* ======================== Инициализация =========================== */

void init_menu_ui(void) {
    if (!lcd_dev_handle) {
        ESP_LOGE(TAG, "LCD faillure!.");
        return;
    }

    ui_sync_from_config();

    s_param_list[UI_PARAM_0] = (setup_param_t){.name = STR_L2_0_0,
                                               .display_type = PARAM_TYPE_INT,
                                               .val_ptr = &s_menu_param.t_setup,
                                               .min_val = 10,
                                               .max_val = 50};

    s_param_list[UI_PARAM_1] = (setup_param_t){.name = STR_L2_0_1,
                                               .display_type = PARAM_TYPE_INT,
                                               .val_ptr = &s_menu_param.t_hysteresis,
                                               .min_val = 1,
                                               .max_val = 10};

    s_param_list[UI_PARAM_2] = (setup_param_t){.name = STR_L2_0_2,
                                               .display_type = PARAM_TYPE_ENUM,
                                               .val_ptr = &s_menu_param.regulator_mode,
                                               .min_val = -1,
                                               .max_val = 3,
                                               .enum_to_str = reg_mode_to_string};

    s_param_list[UI_PARAM_3] = (setup_param_t){.name = STR_L2_0_3,
                                               .display_type = PARAM_TYPE_BOOL,
                                               .val_ptr = &s_menu_param.reserv,
                                               .min_val = 0,
                                               .max_val = 1};

    s_param_list[UI_PARAM_4] = (setup_param_t){.name = STR_L2_0_4,
                                               .display_type = PARAM_TYPE_BOOL,
                                               .val_ptr = &s_menu_param.wifi_enable,
                                               .min_val = 0,
                                               .max_val = 1,
                                               .callback = wifi_action_callback};

    s_param_list[UI_PARAM_5] = (setup_param_t){.name = STR_L2_0_5,
                                               .display_type = PARAM_TYPE_BOOL,
                                               .val_ptr = &s_menu_param.rf433_enable,
                                               .min_val = 0,
                                               .max_val = 1,
                                               .callback = menu_rf433_callback};

    s_param_list[UI_PARAM_6] = (setup_param_t){.name = STR_L2_0_6,
                                               .display_type = PARAM_TYPE_BOOL,
                                               .val_ptr = &s_menu_param.reboot,
                                               .min_val = 0,
                                               .max_val = 1,
                                               .callback = menu_reboot_callback};

    input_queue = xQueueCreate(10, sizeof(input_event_t));

    rotary_encoder_config_t re_config = ROTARY_ENCODER_DEFAULT_CONFIG();
    re_config.pin_a = CONFIG_ENCODER_CLK_PIN;
    re_config.pin_b = CONFIG_ENCODER_DT_PIN;
    re_config.pin_btn = CONFIG_ENCODER_SW_PIN;
    re_config.btn_pressed_level = 0;
    re_config.enable_internal_pullup = true;
    re_config.callback = encoder_event_callback;
    re_config.callback_ctx = NULL;

    ESP_ERROR_CHECK_WITHOUT_ABORT(rotary_encoder_create(&re_config, &re_handle));
    ESP_LOGI(TAG, "EncLib Driver initialized.");

    xTaskCreate(ui_task, "ui_task", 4096, NULL, 4, NULL);
}
