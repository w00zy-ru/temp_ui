#pragma once
#ifndef APP_I2C_DEV_HEADER_H
#define APP_I2C_DEV_HEADER_H

#include "driver/i2c_types.h"
#include "esp_err.h"

#define I2C_FREQUENZY 100000

extern i2c_master_bus_handle_t i2c_bus_handle;
esp_err_t app_i2c_bus_init(void);

#endif
