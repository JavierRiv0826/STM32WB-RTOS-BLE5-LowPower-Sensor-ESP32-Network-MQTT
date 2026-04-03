#include "i2c_bus.h"
#include "esp_log.h"

#define TAG "I2C_BUS"

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define I2C_PORT    I2C_NUM_0

static i2c_master_bus_handle_t bus_handle = NULL;

esp_err_t i2c_bus_init(void)
{
    if (bus_handle != NULL)
        return ESP_OK;

    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    ESP_LOGI(TAG, "I2C bus initialized");

    return ESP_OK;
}

i2c_master_bus_handle_t i2c_bus_get(void)
{
    return bus_handle;
}