#pragma once
#include "esp_err.h"
#ifndef LCDDRV_HEADER_H
#define LCDDRV_HEADER_H

#include "app_i2c_bus.h"
#include "driver/i2c_master.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/* Единственное определение — в ui_driver.c */
extern i2c_master_dev_handle_t lcd_dev_handle;

/* --- ПРИВАТНЫЙ ДРАЙВЕР LCD И РАБОТА С СИСТЕМОЙ --- */
static inline void lcd_write_nibble(uint8_t nibble, uint8_t mode) {
    uint8_t data = (nibble & 0xF0) | mode | 0x08;
    uint8_t buf = data | 0x04;
    i2c_master_transmit(lcd_dev_handle, &buf, 1, 10);
    esp_rom_delay_us(5);
    buf = data & ~0x04;
    i2c_master_transmit(lcd_dev_handle, &buf, 1, 10);
    esp_rom_delay_us(50);
}

static inline void lcd_send_cmd(uint8_t cmd) {
    lcd_write_nibble(cmd & 0xF0, 0);
    lcd_write_nibble((cmd << 4) & 0xF0, 0);
    if (cmd == 0x01 || cmd == 0x02)
        vTaskDelay(pdMS_TO_TICKS(3));
    else
        esp_rom_delay_us(100);
}

static inline void lcd_send_data(uint8_t data) {
    lcd_write_nibble(data & 0xF0, 1);
    lcd_write_nibble((data << 4) & 0xF0, 1);
    esp_rom_delay_us(50);
}

static inline void lcd_set_cursor(uint8_t col, uint8_t row) {
#ifdef LCD_1602
    uint8_t row_offsets[] = {0x00, 0x40};
#else
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
#endif
    lcd_send_cmd(0x80 | (col + row_offsets[row]));
}

static inline void lcd_print_char(const char *str) {
    while (*str)
        lcd_send_data(*str++);
}

#endif
