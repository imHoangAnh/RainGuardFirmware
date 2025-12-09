/**
 * @file system_i2c.c
 * @brief I2C Driver Wrapper Implementation (New I2C Master API)
 */

#include "system_i2c.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "SYSTEM_I2C";
static i2c_master_bus_handle_t bus_handle = NULL;

esp_err_t system_i2c_init(int sda_pin, int scl_pin)
{
    if (bus_handle != NULL) {
        ESP_LOGW(TAG, "I2C already initialized");
        return ESP_OK;
    }

    // Configure I2C master bus
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&bus_config, &bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C master bus init failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2C initialized (SDA: %d, SCL: %d)", sda_pin, scl_pin);
    return ESP_OK;
}

esp_err_t system_i2c_deinit(void)
{
    if (bus_handle == NULL) {
        return ESP_OK;
    }

    esp_err_t err = i2c_del_master_bus(bus_handle);
    if (err == ESP_OK) {
        bus_handle = NULL;
        ESP_LOGI(TAG, "I2C deinitialized");
    }
    return err;
}

esp_err_t system_i2c_write(uint8_t device_addr, uint8_t reg_addr, const uint8_t *data, size_t len)
{
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "I2C not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Create a temporary device handle
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = device_addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    i2c_master_dev_handle_t dev_handle;
    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
    if (err != ESP_OK) {
        return err;
    }

    // Prepare write buffer: [reg_addr, data...]
    uint8_t *write_buf = malloc(len + 1);
    if (write_buf == NULL) {
        i2c_master_bus_rm_device(dev_handle);
        return ESP_ERR_NO_MEM;
    }

    write_buf[0] = reg_addr;
    memcpy(write_buf + 1, data, len);

    // Transmit data
    err = i2c_master_transmit(dev_handle, write_buf, len + 1, I2C_MASTER_TIMEOUT_MS);

    free(write_buf);
    i2c_master_bus_rm_device(dev_handle);

    return err;
}

esp_err_t system_i2c_read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t len)
{
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "I2C not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Create a temporary device handle
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = device_addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    i2c_master_dev_handle_t dev_handle;
    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
    if (err != ESP_OK) {
        return err;
    }

    // Write register address, then read data
    err = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS);

    i2c_master_bus_rm_device(dev_handle);

    return err;
}

