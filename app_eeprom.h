#pragma once
#ifndef _APP_AT24C32_H
#define _APP_AT24C32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

typedef struct {
    uint32_t boot_count;
    uint32_t marker;
    uint16_t config_version;
} eeprom_boot_info_t;

esp_err_t at24c32_eeprom_init(void);
esp_err_t eeprom_check_boot_counter(eeprom_boot_info_t *out_info);

#ifdef __cplusplus
}
#endif

#endif // AT24C32