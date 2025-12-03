/**
 * @file system_i2c.h
 * @brief I2C Driver Wrapper for ESP32-S3
 */

#ifndef SYSTEM_I2C_H
#define SYSTEM_I2C_H

#include "driver/i2c.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// I2C Configuration
#define I2C_MASTER_NUM          I2C_NUM_0
#define I2C_MASTER_FREQ_HZ      100000
#define I2C_MASTER_TX_BUF_LEN   0
#define I2C_MASTER_RX_BUF_LEN   0
#define I2C_MASTER_TIMEOUT_MS   1000

/**
 * @brief Initialize I2C master bus
 * @param sda_pin GPIO pin for SDA
 * @param scl_pin GPIO pin for SCL
 * @return ESP_OK on success
 */
esp_err_t system_i2c_init(int sda_pin, int scl_pin);

/**
 * @brief Deinitialize I2C master bus
 * @return ESP_OK on success
 */
esp_err_t system_i2c_deinit(void);

/**
 * @brief Write data to I2C device
 * @param device_addr I2C device address
 * @param reg_addr Register address
 * @param data Pointer to data buffer
 * @param len Length of data
 * @return ESP_OK on success
 */
esp_err_t system_i2c_write(uint8_t device_addr, uint8_t reg_addr, const uint8_t *data, size_t len);

/**
 * @brief Read data from I2C device
 * @param device_addr I2C device address
 * @param reg_addr Register address
 * @param data Pointer to data buffer
 * @param len Length of data to read
 * @return ESP_OK on success
 */
esp_err_t system_i2c_read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_I2C_H

