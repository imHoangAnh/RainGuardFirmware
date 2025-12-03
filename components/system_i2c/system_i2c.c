/**
 * @file system_i2c.c
 * @brief I2C Driver Wrapper Implementation
 */

#include "system_i2c.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SYSTEM_I2C";
static bool i2c_initialized = false;

esp_err_t system_i2c_init(int sda_pin, int scl_pin)
{
    if (i2c_initialized) {
        ESP_LOGW(TAG, "I2C already initialized");
        return ESP_OK;
    }

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_LEN, I2C_MASTER_TX_BUF_LEN, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return err;
    }

    i2c_initialized = true;
    ESP_LOGI(TAG, "I2C initialized (SDA: %d, SCL: %d)", sda_pin, scl_pin);
    return ESP_OK;
}

esp_err_t system_i2c_deinit(void)
{
    if (!i2c_initialized) {
        return ESP_OK;
    }

    esp_err_t err = i2c_driver_delete(I2C_MASTER_NUM);
    if (err == ESP_OK) {
        i2c_initialized = false;
        ESP_LOGI(TAG, "I2C deinitialized");
    }
    return err;
}

esp_err_t system_i2c_write(uint8_t device_addr, uint8_t reg_addr, const uint8_t *data, size_t len)
{
    if (!i2c_initialized) {
        ESP_LOGE(TAG, "I2C not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    return err;
}

esp_err_t system_i2c_read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t len)
{
    if (!i2c_initialized) {
        ESP_LOGE(TAG, "I2C not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    return err;
}

