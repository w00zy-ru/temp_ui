#include "ui_driver.h"

i2c_master_dev_handle_t lcd_dev_handle = NULL;

esp_err_t init_i2c_lcd(void) {
    if (!i2c_bus_handle)
        return ESP_FAIL;
    i2c_device_config_t dev_cfg = {.dev_addr_length = I2C_ADDR_BIT_LEN_7,
                                   .device_address = CONFIG_LCD_I2C_ADDRESS,
                                   .scl_speed_hz = I2C_FREQUENZY};
    esp_err_t err = i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &lcd_dev_handle);
    if (err != ESP_OK)
        return err;

    vTaskDelay(pdMS_TO_TICKS(50));
    lcd_write_nibble(0x30, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_write_nibble(0x30, 0);
    esp_rom_delay_us(150);
    lcd_write_nibble(0x30, 0);
    lcd_write_nibble(0x20, 0);
    lcd_send_cmd(0x28);
    lcd_send_cmd(0x0C);
    lcd_send_cmd(0x06);
    lcd_send_cmd(0x01);
    return ESP_OK;
}
